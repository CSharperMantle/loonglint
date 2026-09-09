#!/usr/bin/env python3

# SPDX-License-Identifier: GPL-3.0-or-later
# pyright: reportUnusedCallResult=false

"""Attribute LoongLint riscv64 findings to SpiderMonkey JIT tiers.

LoongLint only sees machine code, so it also reports NOP-shaped instructions
that are in fact JIT patcher slots or deliberate markers. With --jitdump those
are recognized from the raw jitdump records and dropped from the actionable
set:

- li_ptr, the ma_liPatchable(ImmPtr) sequence
      lui rd, hi; addi rd, rd, lo; slli rd, rd, 11;
      ori rd, rd, b11; slli rd, rd, 6; ori rd, rd, a6
  is rewritten in place by UpdateLiPtrInstructions, which asserts IsAddi and
  IsOri on the slots. A zero chunk (printed `addi rd, rd, 0` / `mv rd, rd` /
  `ori rd, rd, 0`) is therefore not removable.
- li_constant, the ma_liPatchable(ImmWord) sequence
      lui rd, hi; addiw rd, rd, lo; slli; addi; slli; addi; slli; addi
  is rewritten by UpdateLiConstantInstructions, which asserts IsAddi on the
  final instruction.
- ma_liPatchable(Imm32) emits `lui; addi`, and patchLi32 asserts IsAddi on the
  second instruction.
- wasmMarkCallAsSlow emits `mv(ra, ra)` = `addi ra, ra, 0` (SlowCallMarker
  0x8093); wasmCheckSlowCallsite reads it back at the return address.

The riscv64 backend does not use branch relaxation, so the branch rules stay
actionable.
"""

import re
from bisect import bisect_right
from collections import Counter, defaultdict
from collections.abc import Callable
from pathlib import Path
from typing import Annotated

import typer

from .jitdump2elf import CodeObject, parse_jitdump

TIERS = ["Ion", "Baseline", "Trampoline", "Interpreter", "Wasm", "Other"]

# Rule IDs (riscv: prefix stripped) whose findings may be JIT patcher slots.
NOP_RULES = {"integer/nop"}

RE_FINDING_OFF = re.compile(r"^(.+?):(?P<label>.+)(?P<off>\+0x[0-9a-f]+): .+ \[(?P<rule>[^\]]+)\]$")
RE_FINDING_ABS = re.compile(r"^(.+?):(?P<label>.+?):(?P<abs>0x[0-9a-f]+): .+ \[(?P<rule>[^\]]+)\]$")
RE_FINDING_BARE = re.compile(r"^(.+?):(?P<label>.+): .+ \[(?P<rule>[^\]]+)\]$")

# Removed-instruction line inside a finding block; the absolute address is the
# original JIT address, so a --jitdump context lookup can use it.
RE_REMOVED = re.compile(r"^\t- (0x[0-9a-f]+)\t(\S+)(?:\t(.*))?$")

# Tier prefixes as they appear in the section name. jitdump2elf.py keeps ':'
# in its allowed character set, so the PerfSpewer prefixes survive verbatim.
_TIER_PREFIXES = (
    ("Ion", ("Ion:", "IonIC:")),
    ("Baseline", ("Baseline:", "BaselineIC:", "BaselineICFallback:")),
    ("Interpreter", ("BlinterpOp:", "BaselineInterpreter", "Interpreter:")),
    ("Trampoline", ("Trampoline:", "VMWrapper:")),
    ("Wasm", ("Wasm:",)),
)


def tier_of(symbol: str) -> str:
    for tier, prefixes in _TIER_PREFIXES:
        if symbol.startswith(prefixes):
            return tier
    return "Other"


def rule_key(rule_id: str) -> str:
    """Strip the `riscv:` prefix so the tables stay arch-neutral."""
    return rule_id.removeprefix("riscv:")


# ABI names in register-number order, as printed by the RISC-V MCInstPrinter.
_ABI_REGS = [
    "zero",
    "ra",
    "sp",
    "gp",
    "tp",
    "t0",
    "t1",
    "t2",
    "s0",
    "s1",
    "a0",
    "a1",
    "a2",
    "a3",
    "a4",
    "a5",
    "a6",
    "a7",
    "s2",
    "s3",
    "s4",
    "s5",
    "s6",
    "s7",
    "s8",
    "s9",
    "s10",
    "s11",
    "t3",
    "t4",
    "t5",
    "t6",
]


class PatcherSlots:
    """Recognizes the self-zero instructions that the JIT patchers rewrite in
    place, plus the wasm slow-call marker, from the raw jitdump records.

    riscv64 without the C extension uses fixed 4-byte words, so the neighbors
    of a finding can be decoded with a minimal field extractor.
    """

    OP_IMM = 0x13
    OP_LUI = 0x37
    OP_ADDIW = 0x1B
    F3_ADDI = 0x0
    F3_SLLI = 0x1
    F3_ORI = 0x6

    def __init__(self, jitdump: Path) -> None:
        self._records = sorted(parse_jitdump(jitdump.read_bytes()), key=lambda o: o.addr)
        self._addrs = [o.addr for o in self._records]

    def is_keep_slot(self, mnemonic: str, operands: str, addr: int) -> bool:
        """True if the self-zero `addi`/`mv`/`ori` at |addr| must stay."""
        if mnemonic not in ("addi", "mv", "ori"):
            return False
        parts = [p.strip() for p in (operands or "").split(",")]
        if mnemonic == "mv":
            # `mv rd, rs` prints with two operands and expands to `addi rd, rs, 0`.
            if len(parts) != 2 or parts[0] != parts[1]:
                return False
            parts = [parts[0], parts[1], "0"]
        elif len(parts) != 3 or parts[0] != parts[1]:
            return False
        if mnemonic == "ori" and parts[2] != "0":
            return False
        if parts[0] == "ra" and parts[2] == "0":
            return True  # wasm slow-call marker
        if parts[2] != "0":
            return False
        rd = self._regnum(parts[0])
        if rd is None:
            return False
        for obj in self._find_all(addr):
            rel = addr - obj.addr
            if rel % 4 != 0:
                continue
            prev = self._word_at(obj, rel - 4)
            nxt = self._word_at(obj, rel + 4)
            if self._is_slli(prev, rd, 11) or self._is_slli(prev, rd, 6):
                return True  # li_ptr b11/a6 slot
            if mnemonic != "ori" and (self._is_slli(nxt, rd, 11) or self._is_lui(prev, rd)):
                return True  # li_ptr low-12 slot / ma_liPatchable(Imm32) slot
            if self._is_slli(prev, rd, 12) and self._is_li_constant(obj, rel, rd):
                return True  # li_constant final addi slot
        return False

    def _find_all(self, addr: int):
        i = bisect_right(self._addrs, addr) - 1
        while i >= 0:
            obj = self._records[i]
            if obj.addr <= addr < obj.addr + len(obj.code):
                yield obj
            i -= 1

    @staticmethod
    def _word_at(obj: CodeObject, rel: int) -> int | None:
        if rel < 0 or rel + 4 > len(obj.code):
            return None
        return int.from_bytes(obj.code[rel : rel + 4], "little")

    @staticmethod
    def _fields(w: int) -> tuple[int, int, int, int]:
        return (w >> 7) & 0x1F, (w >> 12) & 0x7, (w >> 15) & 0x1F, (w >> 20) & 0xFFF

    @classmethod
    def _is_slli(cls, w: int | None, rd: int, shamt: int) -> bool:
        if w is None or (w & 0x7F) != cls.OP_IMM:
            return False
        d, f3, _, imm = cls._fields(w)
        return d == rd and f3 == cls.F3_SLLI and (imm & 0x3F) == shamt

    @classmethod
    def _is_lui(cls, w: int | None, rd: int) -> bool:
        if w is None or (w & 0x7F) != cls.OP_LUI:
            return False
        return cls._fields(w)[0] == rd

    @classmethod
    def _is_li_constant(cls, obj: CodeObject, rel: int, rd: int) -> bool:
        """Match LiConstant::isValid(): lui; addiw; slli; addi; slli; addi;
        slli; addi, all writing |rd|.
        """
        if rel < 28:
            return False
        words = [cls._word_at(obj, rel - 4 * k) for k in range(7, -1, -1)]
        if any(w is None for w in words):
            return False
        op: Callable[[int, int], int] = lambda w, opcode: (w & 0x7F) == opcode
        f3: Callable[[int], int] = lambda w: (w >> 12) & 0x7
        return (
            op(words[0], cls.OP_LUI)
            and op(words[1], cls.OP_ADDIW)
            and op(words[2], cls.OP_IMM)
            and f3(words[2]) == cls.F3_SLLI
            and op(words[3], cls.OP_IMM)
            and f3(words[3]) == cls.F3_ADDI
            and op(words[4], cls.OP_IMM)
            and f3(words[4]) == cls.F3_SLLI
            and op(words[5], cls.OP_IMM)
            and f3(words[5]) == cls.F3_ADDI
            and op(words[6], cls.OP_IMM)
            and f3(words[6]) == cls.F3_SLLI
            and op(words[7], cls.OP_IMM)
            and f3(words[7]) == cls.F3_ADDI
            and all(((w >> 7) & 0x1F) == rd for w in words)
        )

    @staticmethod
    def _regnum(name: str) -> int | None:
        if name.startswith("x") and name[1:].isdigit():
            n = int(name[1:])
            return n if n < 32 else None
        return _ABI_REGS.index(name) if name in _ABI_REGS else None


app = typer.Typer(
    add_completion=False,
    help="Attribute LoongLint riscv64 findings to SpiderMonkey JIT tiers.",
)


def show(rows: list[tuple[str, Counter[str]]]) -> None:
    """Print a rule x tier table, each column justified by its longest cell."""
    header = ["Rule", *TIERS, "Total"]
    cells = [
        [rule, *(str(ct.get(t, 0)) for t in TIERS), str(sum(ct.values()))] for rule, ct in rows
    ]
    widths = [max(map(len, col)) for col in zip(*([header] + cells))]
    for row in [header, *cells]:
        print(
            "  ".join(c.rjust(w) if i else c.ljust(w) for i, (c, w) in enumerate(zip(row, widths)))
        )


@app.command()
def main(
    findings: Annotated[
        Path, typer.Argument(help="LoongLint stdout file", exists=True, dir_okay=False)
    ],
    jitdump: Annotated[
        Path | None,
        typer.Option(
            "--jitdump",
            help="Source jit-*.dump for NopRule patcher-slot context filtering",
            exists=True,
            dir_okay=False,
        ),
    ] = None,
    sites: Annotated[
        Path | None,
        typer.Option(
            "--sites",
            help="Write actionable findings as TSV (rule, tier, symbol, addr, mnemonic, operands)",
            file_okay=True,
            dir_okay=False,
        ),
    ] = None,
) -> None:
    slots = PatcherSlots(jitdump) if jitdump else None

    type_tier: defaultdict[str, Counter[str]] = defaultdict(Counter)
    kept: defaultdict[str, Counter[str]] = defaultdict(Counter)
    site_rows: list[tuple[str, str, str, int, str, str]] = []
    symbol = ""
    pending: tuple[str, str] | None = None  # (rule, tier) awaiting its removed line
    with open(findings, errors="replace") as f:
        for line in f:
            m = (
                RE_FINDING_OFF.match(line)
                or RE_FINDING_ABS.match(line)
                or RE_FINDING_BARE.match(line)
            )
            if m:
                symbol, rule = m.group("label"), m.group("rule")
                tier = tier_of(symbol)
                type_tier[rule_key(rule)][tier] += 1
                pending = (rule, tier)
                continue
            if pending and (m := RE_REMOVED.match(line)):
                rule, tier = pending
                pending = None
                key = rule_key(rule)
                mnemonic, operands = m.group(2), (m.group(3) or "")
                addr = int(m.group(1), 16)
                if key in NOP_RULES and slots and slots.is_keep_slot(mnemonic, operands, addr):
                    kept[key][tier] += 1
                else:
                    site_rows.append((key, tier, symbol, addr, mnemonic, operands))

    print("== Rule x Tier ==")
    show(sorted(type_tier.items(), key=lambda kv: -sum(kv[1].values())))

    print()
    print("== Actionable items ==")
    actionable: list[tuple[str, Counter[str]]] = []
    for rule, ct in type_tier.items():
        remaining = ct - kept[rule]
        if sum(remaining.values()):
            actionable.append((rule, remaining))
    actionable.sort(key=lambda kv: -sum(kv[1].values()))
    show(actionable)
    print()
    print(f"\tTotal actionable: {sum(sum(c.values()) for _, c in actionable)}")
    if sites:
        with open(sites, "w") as f:
            f.writelines("\t".join(map(str, row)) + "\n" for row in site_rows)
        typer.echo(f"{len(site_rows)} actionable sites -> {sites}", err=True)


if __name__ == "__main__":
    app()
