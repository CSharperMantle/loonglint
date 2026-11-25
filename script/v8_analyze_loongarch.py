#!/usr/bin/env python3

# SPDX-License-Identifier: GPL-3.0-or-later
# pyright: reportUnusedCallResult=false

import re
from collections import Counter, defaultdict
from pathlib import Path
from typing import Annotated

import typer

TIERS = [
    "Baseline",
    "Maglev",
    "Maglev (OSR)",
    "TurboFan",
    "TurboFan (OSR)",
    "Interpreted",
    "wasm (Liftoff)",
    "wasm (TurboFan)",
    "JS (untiered)",
    "Builtin",
    "BytecodeHandler",
    "RegExp",
    "Other",
]

JS_MARKERS = (
    ("o+", "Maglev (OSR)"),
    ("o*", "TurboFan (OSR)"),
    ("+'", "Maglev"),
    ("*'", "TurboFan"),
    ("+", "Maglev"),
    ("*", "TurboFan"),
    ("^", "Baseline"),
    ("~", "Interpreted"),
)

WASM_SUFFIXES = (
    ("-liftoff", "wasm (Liftoff)"),
    ("-turbofan", "wasm (TurboFan)"),
)

STRUCTURAL_POLICY = (
    (
        "loongarch:integer/nop",
        "ori Rd, Rd, 0",
        "fixed-shape constant load slot (MacroAssembler::li)",
    ),
    (
        "loongarch:integer/nop-la64",
        "addi.d $sp, $sp, 0",
        "Liftoff frame-size patch slot (LiftoffAssembler::PrepareStackFrame)",
    ),
)

RE_FINDING_OFF = re.compile(r"^(.+?):(?P<label>.+)(?P<off>\+0x[0-9a-f]+): .+ \[(?P<rule>[^\]]+)\]$")
RE_FINDING_ABS = re.compile(r"^(.+?):(?P<label>.+?):(?P<abs>0x[0-9a-f]+): .+ \[(?P<rule>[^\]]+)\]$")
RE_FINDING_BARE = re.compile(r"^(.+?):(?P<label>.+): .+ \[(?P<rule>[^\]]+)\]$")

RE_REMOVED = re.compile(r"^\t- 0x[0-9a-f]+\t(?:\s+\t)?(\S+)(?:\t(.*))?$")


def bucket_of(label: str) -> str:
    """Classify a finding label into the tier buckets the converter encodes."""
    tag, sep, rest = label.partition(":")
    if sep and tag == "JS":
        for marker, tier in JS_MARKERS:
            if rest.startswith(marker):
                return tier
        name = rest.split(" ", 1)[0]
        for suffix, tier in WASM_SUFFIXES:
            if name.endswith(suffix):
                return tier
        return "JS (untiered)"
    if tag in ("Builtin", "BytecodeHandler"):
        return tag
    if label.startswith("RegExp."):
        return "RegExp"
    return "Other"


def structural_reason(rule: str, mnemonic: str, operands: str) -> str | None:
    """Return the emitter fact that makes a removed word structural, if any."""
    parts = [part.strip() for part in operands.split(",")]
    if (
        rule == "loongarch:integer/nop"
        and mnemonic == "ori"
        and len(parts) == 3
        and parts[0] == parts[1]
        and parts[2] == "0"
    ):
        return "fixed-shape constant load slot (MacroAssembler::li)"
    if (
        rule == "loongarch:integer/nop-la64"
        and mnemonic == "addi.d"
        and len(parts) == 3
        and parts[0] == parts[1] == "$sp"
        and parts[2] == "0"
    ):
        return "Liftoff frame-size patch slot (LiftoffAssembler::PrepareStackFrame)"
    return None


app = typer.Typer(
    add_completion=False,
    help="Attribute LoongLint findings from a V8 jitdump ELF to V8 tiers.",
)


def show(rows: list[tuple[str, Counter[str]]]) -> None:
    """Print a rule x tier table, each column justified by its longest cell."""
    header = ["Rule", *TIERS, "Total"]
    cells = [
        [rule, *(str(count.get(tier, 0)) for tier in TIERS), str(sum(count.values()))]
        for rule, count in rows
    ]
    widths = [max(map(len, column)) for column in zip(*([header] + cells))]
    for row in [header, *cells]:
        print(
            "  ".join(
                cell.rjust(width) if index else cell.ljust(width)
                for index, (cell, width) in enumerate(zip(row, widths))
            )
        )


@app.command()
def main(
    findings: Annotated[
        Path, typer.Argument(help="LoongLint stdout file", exists=True, dir_okay=False)
    ],
) -> None:
    type_tier: defaultdict[str, Counter[str]] = defaultdict(Counter)
    structural: defaultdict[str, Counter[str]] = defaultdict(Counter)
    pending: tuple[str, str] | None = None
    with open(findings, errors="replace") as stream:
        for line in stream:
            match = (
                RE_FINDING_OFF.match(line)
                or RE_FINDING_ABS.match(line)
                or RE_FINDING_BARE.match(line)
            )
            if match:
                rule = match.group("rule")
                tier = bucket_of(match.group("label"))
                type_tier[rule][tier] += 1
                pending = (rule, tier)
                continue
            if pending and (removed := RE_REMOVED.match(line)):
                rule, tier = pending
                pending = None
                reason = structural_reason(rule, removed.group(1), removed.group(2) or "")
                if reason:
                    structural[rule][tier] += 1

    print("== Policy ==")
    print("\tbucket table: converter `tier_of` vocabulary, wasm split on suffix")
    print("\tsite identity: findings")
    for rule, shape, reason in STRUCTURAL_POLICY:
        print(f"\tsubtracted: {rule}: {shape} -> {reason}")

    print()
    print("== Rule x Tier ==")
    show(sorted(type_tier.items(), key=lambda item: -sum(item[1].values())))

    print()
    print("== Actionable items ==")
    actionable: list[tuple[str, Counter[str]]] = []
    for rule, count in type_tier.items():
        remaining = count - structural[rule]
        if sum(remaining.values()):
            actionable.append((rule, remaining))
    actionable.sort(key=lambda item: -sum(item[1].values()))
    show(actionable)
    print()
    print(f"\tTotal actionable: {sum(sum(count.values()) for _, count in actionable)}")


if __name__ == "__main__":
    app()
