# Fixtures and documentation

## 1. Test suite layout

- `test/` holds lit fixtures. `test/CMakeLists.txt` runs `add_lit_testsuite(check-loonglint ...)` over the build directory — **lit discovers new fixture files automatically; adding a test never requires a CMake edit.**
- Fixture kinds, by filename:
   - `rule_<category>_<id>_match.s` — positive: the rule must fire N times.
   - `rule_<category>_<id>_mismatch.s` — negative: near-misses must produce zero findings.
   - Variants when needed: `_la32_match` / `_la32_mismatch` (LA32-legal inputs), `_boundary_mismatch` (off-by-one immediates, bounds). See `rule_integer_shift_add_alsl_d_*` for precedents.
- Other fixtures (`cli_*`, `elf*`, `raw*`, `smoke.test`) cover the CLI and decoder plumbing; a rule change should not need them.
- Tools available to RUN lines are declared as `DEPENDS` in `test/CMakeLists.txt` (`llvm-mc`, `FileCheck`, `not`, `yaml2obj`, `llvm-objcopy`, `loonglint`); `ld.lld` comes from the surrounding LLVM build and must exist on PATH.

## 2. Match fixture format

```s
## One-line statement of which forms this file covers.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64-unknown-linux -filetype=obj %s -o %t.o
# RUN: ld.lld --entry=_start %t.o -o %t.exe
# RUN: not loonglint %t.exe | FileCheck %s

## Optional per-group comments for sub-families.
# CHECK-COUNT-6: [memory/unsigned-load-pick]
# CHECK: 6 finding(s)
# CHECK: 6 memory/unsigned-load-pick

.text
.globl _start
_start:
  ld.bu      $t0, $a0, 0
  bstrpick.d $t0, $t0, 7, 0
  ...
```

- The `RUN:` and `CHECK*:` comment lines are llvm-lit and FileCheck directives. These are to be prefixed with one hash symbol. Refer to <https://llvm.org/docs/CommandGuide/FileCheck.html> or `llvm/docs/CommandGuide/FileCheck.rst` in the LLVM tree for details about the usage of FileCheck.
- Other comment lines are genuinely comments. Prefix them with *two* hash symbols to distinguish them from FileCheck directives.
- The three `CHECK*:` lines are a contract: total count, total findings, and per-rule count. All three must agree with the number of matching windows in the file.
- `not` is required because `loonglint` exits nonzero when it reports findings.
- For LA32 coverage use `-triple=loongarch32-unknown-linux` and only LA32S-legal instructions (which means no `.D` forms, no 64-only immediatesl. Refer to the Manual for details).

## 3. Mismatch fixture format and the zero-findings discipline

```s
## Reject <what and why>.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64-unknown-linux -filetype=obj %s -o %t.o
# RUN: ld.lld --entry=_start %t.o -o %t.exe
# RUN: loonglint %t.exe | FileCheck %s

# CHECK: 0 finding(s)
```

The assertion is **zero findings from every rule in the registry**, not just yours. This is where rule work most often goes wrong:

- Every pair of adjacent instructions is a candidate window for every 2-instruction rule. A negative for your rule can be a positive for another. Real example: `ld.bu $t0, $a0, 0` followed by `bstrpick.d $t0, $t0, 7, 0` is a valid negative for `memory/load-zero-extend` (unsigned load) but matches `memory/unsigned-load-pick` exactly.
- Before finalizing a negative, walk the registry in `RuleManager` and check the window against each rule that shares either opcode. Change a register, an immediate, or the opcode until nothing fires — and know *why* nothing fires.
- Prefer negatives that each isolate one constraint of your rule (wrong immediate, wrong destination, aliased source, off-by-one bound) rather than one catch-all blob.

FileCheck notes: CHECK directives collapse runs of whitespace, so indentation never needs to match; do not use `{{.*}}` patterns — exact counts and exact ID lines only.

## 4. RULES.md discipline

RULES.md is the user-facing catalog. One `## XRule` section per rule (`category/name` in the heading), sections ordered exactly like registration in `RuleManager.cpp` — insert new sections at the matching position.

Section structure:

````markdown
## `XRule` (`category/name`)

```asm
<pattern asm, lowercase mnemonics, matching what fixtures contain>
# ->
<replacement>
```

### Constraints

<arch availability, then bullet list; chain bullets with "and"; capitalize the first character of each item>

<one-paragraph semantic justification>

### Evidence

* <exact URL permalink into LLVM/GCC source that emits or peephole-optimizes this shape>
````

Evidence policy:

- Evidence exists to show the pattern is real compiler behavior, not to re-teach instruction semantics. Never paraphrase the architecture manual there — well-known semantics add no value.
- Either link the exact source location (LLVM `LoongArchInstrInfo.td` / `LoongArchISelDAGToDAG.cpp` patterns, GCC `loongarch.md` peepholes) or omit the Evidence section entirely.
- Pattern asm blocks use the same lowercase mnemonic style as fixtures; comments inside the block explain accepted variants (`# srai.d is accepted in place of srli.d ...`).

If the user keeps a proposal/backlog document for candidate patterns, ask where it lives and mark the newly shipped rule there; do not guess at file names.

## 5. Unit tests

`unittests/MCInstMatcherTest.cpp` (built as `LoongLintUnitTests`, run by `check-loonglint`) covers matcher behavior directly. Only add tests here if the new case is genuinely a matcher DSL exercise. Generally speaking, this would be rare.
