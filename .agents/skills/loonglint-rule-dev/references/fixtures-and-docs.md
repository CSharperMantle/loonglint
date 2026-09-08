# Fixtures and documentation

## 1. Layout

- LoongArch fixtures live in `test/LoongArch/`; arch-independent driver fixtures sit in `test/`. `add_lit_testsuite(check-loonglint ...)` discovers files recursively -- adding a test never needs a CMake edit.
- Names: `rule_<category>_<id>_match.s` (positive, fires N times), `rule_<category>_<id>_mismatch.s` (near-misses, zero findings); variants `_la32_*`, `_boundary_mismatch`. Precedents: `rule_integer_shift_add_alsl_d_*`.
- `cli_*`/`elf*`/`raw*` fixtures cover CLI and decoder plumbing; rule work should not touch them. RUN-line tools are `DEPENDS` in `test/CMakeLists.txt`; `ld.lld` must exist on PATH.

## 2. Match fixture

```s
## One-line statement of which forms this file covers.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64-unknown-linux -filetype=obj %s -o %t.o
# RUN: ld.lld --entry=_start %t.o -o %t.exe
# RUN: not loonglint %t.exe | FileCheck %s

# CHECK-COUNT-6: [loongarch:memory/unsigned-load-pick]
# CHECK: 6 finding(s)
# CHECK: 6 loongarch:memory/unsigned-load-pick

.text
.globl _start
_start:
  ld.bu      $t0, $a0, 0
  bstrpick.d $t0, $t0, 7, 0
  ...
```

- `#`-prefixed lines are lit/FileCheck directives; real comments use `##`. `not` is required -- `loonglint` exits nonzero on findings.
- The three `CHECK*` lines are a contract: total count, findings count, per-rule count. Finding IDs are `loongarch:`-prefixed, including inside `-E` patterns.
- LA32: `-triple=loongarch32-unknown-linux`, LA32S-legal instructions only.

## 3. Mismatch fixture and the zero-findings discipline

```s
## Reject <what and why>.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64-unknown-linux -filetype=obj %s -o %t.o
# RUN: ld.lld --entry=_start %t.o -o %t.exe
# RUN: loonglint %t.exe | FileCheck %s

# CHECK: 0 finding(s)
```

Zero findings means zero from **every** rule, not just yours:

- Adjacent pairs are candidate windows for every 2-instruction rule. Real example: `ld.bu $t0, $a0, 0` + `bstrpick.d $t0, $t0, 7, 0` is a valid negative for `loongarch:memory/load-zero-extend` but matches `loongarch:memory/unsigned-load-pick`.
- Before finalizing, walk the registry in `LoongArchSpec::createRules()` against every rule sharing an opcode; adjust register/immediate/opcode until nothing fires, and know why.
- Isolate one constraint per negative (wrong immediate, destination, aliasing, off-by-one).
- FileCheck collapses whitespace; use exact counts and exact ID lines, never `{{.*}}`.

## 4. RULES.md

One `## XRule` section per rule (`loongarch:category/name` heading), ordered like `createRules()`.

````markdown
## `XRule` (`loongarch:category/name`)

```asm
<pattern asm, lowercase, matching fixtures>
# ->
<replacement>
```

### Constraints

<availability, then bullets chained with "and">

<semantic justification>

### Evidence

* <exact permalink into LLVM/GCC source, or omit the section>
````

Evidence shows the pattern is real compiler behavior; never paraphrase the ISA manual there. Pattern asm matches fixture style. If the user keeps a candidate/backlog list, ask where and mark the rule shipped; do not guess file names.

## 5. Unit tests

`unittests/MCInstMatcherTest.cpp` covers the matcher DSL directly; add tests only for genuine DSL exercises. Generally speaking, adding new cases would be rare.
