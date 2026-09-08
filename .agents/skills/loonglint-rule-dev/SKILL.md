---
name: loonglint-rule-dev
description: >
  Workflow, conventions, and soundness rules for LoongLint peephole rules (with LoongArch32S/LoongArch64 as example): For changes to src/Rules/$ARCH, include/loonglint/Rules/$ARCH, using LowLevelInstMatcherDSL matchers, fixtures in test/LoongArch, and RULES.md docs. Use when adding, extending, debugging, or documenting a rule, pattern, or fixture; *not* for engine, decoder, or CLI work.
license: GPL-3.0-or-later
---

# LoongLint rule development

LoongLint is a peephole linter for LoongArch64 (LA64) and LoongArch32S (LA32) ELF and raw binaries. It disassembles linearly and slides fixed-size windows; each Rule matches one window and reports a finding with a replacement sequence. No liveness analysis, no CFG, no relocation awareness -- soundness must hold inside the peephole alone.

## 0. Read before writing code

Read [references/matcher-dsl.md](references/matcher-dsl.md) before writing your first `match()` and before debugging a matcher. Read [references/fixtures-and-docs.md](references/fixtures-and-docs.md) before writing fixtures or touching RULES.md.

## 1. Workflow

1. **Verify the pattern against the manual.** Derive every semantic fact from the "LoongArch Reference Manual - Volume 1" <https://loongson.github.io/LoongArch-Documentation/LoongArch-Vol1-EN.pdf> -- never from memory. Reject patterns that need cross-instruction state the peephole cannot see.
2. **Name and ID.** Class `XRule` in `namespace loonglint::LoongArch`; ID `"loongarch:category/name"` (`loongarch:integer/`, `loongarch:memory/`, `loongarch:control/`).
3. **Implement** `include/loonglint/Rules/LoongArch/XRule.hpp` + `src/Rules/LoongArch/XRule.cpp`. Copy the closest exemplar, not a template.
4. **Register** in `LoongArchSpec::createRules()` (`src/LoongArch/LoongArchSpec.cpp`): include (alphabetical) + `Rules.emplace_back(std::make_unique<XRule>());` -- `(*this)` if variant-gated. Order is finding order. Do not touch `RuleManager`; it pulls the list from the spec and knows no concrete rules.
5. **List the source** in `add_llvm_library(LoongLint ...)` in `CMakeLists.txt` (alphabetical).
6. **Fixtures** under `test/LoongArch/`: match + mismatch at minimum; LA32 variant if LA32-legal.
7. **Document** in RULES.md, section in registration order.
8. **Backlog**: if the user keeps a candidate list, ask where and mark the rule shipped.
9. **Format, build, test**: `clang-format -i` touched files; build + suite below. Fix clang-tidy findings, do not suppress.
10. **Report and stop.** Never mutate VCS state unless asked.

## 2. Conventions

Exemplars -- read the closest before writing:

- Deletion -> `src/Rules/LoongArch/UnsignedLoadPickRule.cpp`
- Fusion with immediates/range checks -> `src/Rules/LoongArch/AddressLoadRule.cpp`
- Two match orders -> `src/Rules/LoongArch/BitExtractRule.cpp`
- Matcher-reuse dataflow constraint -> `src/Rules/LoongArch/ShiftMaskRule.cpp`
- Tuple-table variant sweep -> `src/Rules/LoongArch/LoadZeroExtendRule.cpp`
- Spec-injected gating -> `src/Rules/LoongArch/NopLA64Rule.cpp` (whole rule), `ShiftMaskRule.cpp` (per-arm)

File shape (drop ctor/member/spec-include when the rule does not gate):

```cpp
#include "loonglint/LoongArch/LoongArchSpec.hpp"
#include "loonglint/Rule.hpp"

namespace loonglint::LoongArch {

namespace LoongArch = ::llvm::LoongArch; // required: shadows llvm::LoongArch (see Gotchas)

class XRule final : public Rule {
  public:
    explicit XRule(const LoongArchSpec &LoongAS);      // gating rules only
    llvm::StringRef getID() const override;            // "loongarch:category/name"
    llvm::StringRef getDescription() const override;   // imperative: "fold X into Y"
    unsigned getInstructionCount() const override;     // window size
    bool shouldRun(const Context &) const override;    // gating rules only
    std::optional<Match> match(llvm::ArrayRef<Instruction> Instructions,
                               const Context &Ctx) const override;

  private:
    const LoongArchSpec &LoongAS;
};

} // namespace loonglint::LoongArch
```

- **Gating.** `Context` carries only `MCInstrAnalysis`; variant knowledge comes from the injected spec. Whole-rule -> `shouldRun { return LoongAS.is64(); }`; per-arm -> check `LoongAS.is64()` inside `match()`. "LA32" means LA32S: `BSTRPICK.W`, `ANDN`/`ORN`, `BEQZ`/`BNEZ`, byte/bit ops are legal without extra gating; no `.D`, no 64-only immediates. Some instructions differ per variant: `ADDI.W Rd, Rd, 0` is a no-op on LA32 but a word-to-double sign-extension on LA64.
- **No finding** -> `return std::nullopt`. A `Match` replaces the whole window: deletion keeps the surviving instruction (`Result.Replacement.emplace_back(F)`), fusions build with `MCInstBuilder` in canonical TableGen operand order.
- **Immediates.** TableGen unsigned bitfields (`BSTRPICK` msb/lsb) still arrive as `int64_t` `Imm`. Recombined offsets follow encoder rules: load/store `isInt<12>`; `LDPTR` decodes pre-scaled (`isShiftedInt<14, 2>`).
- **Windows.** Assert `Instructions.size()` at the top of `match()`; windows slide by one, so a 2-instruction rule sees every consecutive pair.
- **Style.** `.clang-format` (LLVM-based, 4-space, column 100). PascalCase; no multiple definitions-with-initializers on one line. Our mnemonics UPPERCASE (`ADD.D`); fixtures keep llvm-mc's lowercase verbatim. Do not hard-wrap Markdown.

## 3. Soundness

Reject unless the pattern alone proves the liveness requirement inside the window.

- Same-destination rewrites only; fusions must overwrite the temporary with the replacement's own destination.
- `$zero` ignores writes and reads 0: never route addresses through it (`addi.d $zero` + `ld.* $zero, $zero, 0` faults differently); see `AddressLoadRule`'s `R0` rejection.
- Reject semantics-changing aliasing (`ShiftMaskRule`'s count==value rejection).
- Uncertain -> reject. False negatives are cheap; false positives burn trust.

## 4. Gotchas

- Matcher captures persist after success and act as equality constraints on reuse; failures roll back. Reuse deliberately, fresh matchers for independent attempts.
- `LDPTR` offsets arrive pre-scaled; `ALSL`'s assembly immediate is the real shift 1..4 (encoded minus one). Confirm against the LLVM source and the manual.
- A mismatch fixture can accidentally match a different rule; check negatives against every overlapping rule.
- Orderings: `LoongArchSpec.cpp` includes and the CMake source list are alphabetical; `createRules()` order drives finding order and RULES.md order.
- Probe operand layouts with `llvm-mc` before writing fixtures.

## 5. Build and test

```sh
cmake -S "$LLVM_PROJECT"/llvm -B build -G Ninja \
  -DLLVM_EXTERNAL_PROJECTS='loonglint' \
  -DLLVM_EXTERNAL_LOONGLINT_SOURCE_DIR="$(pwd)" \
  -DLLVM_TARGETS_TO_BUILD='LoongArch' \
  -DCMAKE_BUILD_TYPE=Debug \
  -DLLVM_ENABLE_ASSERTIONS=ON \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

ninja -C build loonglint
ninja -C build check-loonglint   # lit fixtures + unit tests; discovery is automatic
```
