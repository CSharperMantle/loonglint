import re
from collections import Counter, defaultdict
from pathlib import Path
from typing import Annotated

import typer

# Rule sets: forced (N/A) rules vs. actionable ones. The reason strings are
# shown in the N/A section.
FORCED = {
    "control/degenerate-branch": "branch relaxation",
    "control/branch-to-next": "branch relaxation",
    "integer/nop": "ma_liPatchable reloc slot",
    "integer/nop-la32": "ma_liPatchable reloc slot",
    "integer/nop-la64": "ma_liPatchable reloc slot",
}
TIERS = ["Ion", "Baseline", "Trampoline", "Interpreter", "Other"]

# loonglint finding line: `file:section:0xaddr: description [rule]`
# The section name is the sanitized PerfSpewer name and may itself contain
# ':' (jitdump2elf.py keeps ':' in its allowed set), so anchor on the ':0x'
# that precedes the address rather than splitting on every ':'.
RE_FINDING = re.compile(r"^.+?:(.+?):0x([0-9a-f]+): .+ \[([^\]]+)\]")

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
    with open(findings, errors="replace") as f:
        for line in f:
            m = RE_FINDING.match(line)
            if not m:
                continue
            section, _addr, rule = m.group(1), int(m.group(2), 16), m.group(3)
            tier = next((t for t, ps in _TIER_PREFIXES if section.startswith(ps)), "Other")
            type_tier[rule][tier] += 1

    print("== Rule x Tier ==")
    show(sorted(type_tier.items(), key=lambda kv: -sum(kv[1].values())))

    print()
    print("== Actionable items ==")
    actionable = sorted(
        ((r, c) for r, c in type_tier.items() if r not in FORCED),
        key=lambda kv: -sum(kv[1].values()),
    )
    show(actionable)
    print()
    print(f"\tTotal actionable: {sum(sum(c.values()) for _, c in actionable)}")


if __name__ == "__main__":
    app()
