import re
from collections import Counter, defaultdict
from pathlib import Path
from typing import Annotated

import typer

# Rules fully forced (N/A): branch relaxation artifacts.
FORCED = {
    "control/degenerate-branch": "branch relaxation",
    "control/branch-to-next": "branch relaxation",
}
# NOP-shaped rules whose findings may be ma_liPatchable low-12-bit slots.
NOP_RULES = {"integer/nop", "integer/nop-la32", "integer/nop-la64"}
TIERS = ["Ion", "Baseline", "Trampoline", "Interpreter", "Other"]

# loonglint finding line: `file:section:0xaddr: description [rule]`
# The section name is the sanitized PerfSpewer name and may itself contain
# ':' (jitdump2elf.py keeps ':' in its allowed set), so anchor on the ':0x'
# that precedes the address rather than splitting on every ':'.
RE_FINDING = re.compile(r"^.+?:(.+?):0x[0-9a-f]+: .+ \[([^\]]+)\]")

# Removed-instruction line inside a finding block.
RE_REMOVED = re.compile(r"^\t- 0x[0-9a-f]+\t(\S+)(?:\t(.*))?$")


def is_li_slot(mnemonic: str | None, operands: str | None) -> bool:
    """True if the instruction is the `ori Rd, Rd, 0` low-12-bit slot that
    loong64 ma_liPatchable emits (lu12i.w + ori + lu32i.d [+ lu52i.d]); the
    ori must stay so the JIT patcher can rewrite the low 12 bits."""
    if mnemonic != "ori":
        return False
    parts = [p.strip() for p in (operands or "").split(",")]
    return len(parts) == 3 and parts[0] == parts[1] and parts[2] == "0"


def is_wasm_slow_call_marker(mnemonic: str | None, operands: str | None) -> bool:
    """True if the instruction is the intentional wasm slow-call marker
    emitted by MacroAssemblerLOONG64::wasmMarkCallAsSlow(): `mov(ra, ra)`
    = `ori ra, ra, 0` (SlowCallMarker 0x03800021). wasmCheckSlowCallsite()
    reads this word at the return address to detect slow-path call_indirect
    sites, so it must not be removed. Same idiom as riscv64's `mv(ra, ra)`.

    Shape-wise this is a subset of is_li_slot, so it is already filtered in
    practice; this filter documents the intent and keeps the marker excluded
    if the li-slot rule is ever tightened."""
    if mnemonic != "ori":
        return False
    parts = [p.strip() for p in (operands or "").split(",")]
    return len(parts) == 3 and parts[0] == "$ra" == parts[1] and parts[2] == "0"


# Tier prefixes as they appear in the section name. jitdump2elf.py keeps ':'
# in its allowed character set, so the PerfSpewer prefixes survive verbatim.
_TIER_PREFIXES = (
    ("Ion", ("Ion:", "IonIC:")),
    ("Baseline", ("Baseline:", "BaselineIC:", "BaselineICFallback:")),
    ("Interpreter", ("BlinterpOp:", "BaselineInterpreter", "Interpreter:")),
    ("Trampoline", ("Trampoline:", "VMWrapper:")),
)

app = typer.Typer(
    add_completion=False,
    help="Attribute LoongLint findings to SpiderMonkey JIT tiers and filter out non-applicable ones.",
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
) -> None:
    type_tier: defaultdict[str, Counter[str]] = defaultdict(Counter)
    li_slots: defaultdict[str, Counter[str]] = defaultdict(Counter)
    pending: tuple[str, str] | None = None  # (rule, tier) awaiting its removed line
    with open(findings, errors="replace") as f:
        for line in f:
            m = RE_FINDING.match(line)
            if m:
                section, rule = m.group(1), m.group(2)
                tier = next((t for t, ps in _TIER_PREFIXES if section.startswith(ps)), "Other")
                type_tier[rule][tier] += 1
                pending = (rule, tier)
                continue
            if pending and (m := RE_REMOVED.match(line)):
                rule, tier = pending
                pending = None
                if rule in NOP_RULES and (
                    is_li_slot(m.group(1), m.group(2))
                    or is_wasm_slow_call_marker(m.group(1), m.group(2))
                ):
                    li_slots[rule][tier] += 1

    print("== Rule x Tier ==")
    show(sorted(type_tier.items(), key=lambda kv: -sum(kv[1].values())))

    print()
    print("== Actionable items ==")
    actionable: list[tuple[str, Counter[str]]] = []
    for rule, ct in type_tier.items():
        if rule in FORCED:
            continue
        remaining = ct - li_slots[rule]
        if sum(remaining.values()):
            actionable.append((rule, remaining))
    actionable.sort(key=lambda kv: -sum(kv[1].values()))
    show(actionable)
    print()
    print(f"\tTotal actionable: {sum(sum(c.values()) for _, c in actionable)}")


if __name__ == "__main__":
    app()
