---
name: loonglint-rule-dev
description: Implement, extend, or debug LoongLint peephole rules in this repository. Rule classes under include/loonglint/Rules and src/Rules, the LowLevelInstMatcherDSL, RuleManager registration, lit fixtures under test/, and RULES.md documentation. Use this skill whenever the task touches any of those, or when the user asks to add, extend or debug a rule or a pattern.
license: GPL-3.0-or-later
---

# LoongLint rule development

LoongLint is a peephole linter for LoongArch64 (LA64) and LoongArch32S (LA32) ELF and raw binaries. It disassembles linearly and slides fixed-size windows over the decoded stream; each Rule matches a fixed-size window (hence the name "peephole") and reports a finding with a suggested replacement sequence. There is no liveness analysis, no CFG, and no relocation awareness -- soundness arguments must hold inside the peephole alone.

## 0. Read before writing code

- Rules are implemented using the LowLevelInstMatcherDSL. Read [references/matcher-dsl.md](references/matcher-dsl.md) before writing your first `match()`, and again before debugging a matcher that seems wrong. The DSL is easy to get wrong without prior knowledge.
- Read [references/fixtures-and-docs.md](references/fixtures-and-docs.md) before writing fixtures or updating RULES.md.

## 1. Workflow

Work top to bottom; do not skip Step 1.

1. **Verify the pattern against the architecture manual.** Derive every semantic fact (operand widths, shift-amount encodings, sign/zero extension behavior, LA32 vs LA64 availability per the instruction existence table) from the "LoongArch Reference Manual - Volume 1" at <https://loongson.github.io/LoongArch-Documentation/LoongArch-Vol1-EN.pdf> -- never from memory. Reject the pattern if the manual shows it needs cross-instruction state the peephole cannot see.
2. **Pick the name and ID.** Class `XRule` in PascalCase; ID is `"category/name"` with a lowercase slug. Categories currently in use:
   - `integer/`
   - `memory/`
   - `control/`
3. **Implement** `include/loonglint/Rules/XRule.hpp` and `src/Rules/XRule.cpp` following the conventions below. Copy the shape of the closest exemplar rule, not a generic template.
4. **Register** the rule in `src/RuleManager.cpp`: add the include (alphabetical order) and a `registerRule(std::make_unique<XRule>())` call. Registration order is finding output order -- place the rule next to related rules. Rule IDs must be unique (asserted in debug builds).
5. **List the source** in the `add_llvm_tool(loonglint ...)` list in `CMakeLists.txt` (alphabetical order).
6. **Write fixtures** under `test/`: a match (positive) fixture and a mismatch (negative) fixture at minimum, plus an LA32 variant if the rule runs on LA32. See [references/fixtures-and-docs.md](references/fixtures-and-docs.md) for exact formats and the zero-findings discipline.
7. **Document in RULES.md**: one section per rule, placed in registration order. See [references/fixtures-and-docs.md](references/fixtures-and-docs.md) for the section structure and the evidence policy.
8. **Update the candidate list**, _if_ the user maintains one: ask the user where their proposal/backlog notes live and mark the item shipped there. Do not guess at file names.
9. **Format, build, test**: run `clang-format -i` on every touched C++ file, then build and run the suite (commands below). clang-tidy runs during compilation -- _fix_ its findings rather than suppressing them.
10. **Report and stop.** Inform the user that the task is complete. Summarize what was done and what the user still needs to do. Do NOT mutate any version control system states unless explicitly asked to do so by the user.

## 2. Implementation conventions

### 2.1. Exemplars

Read the one closest to your shape before writing:

- Deletion (replacement = surviving instruction) -> `src/Rules/UnsignedLoadPickRule.cpp`
- Fusion with re-encoded immediates and range checks -> `src/Rules/AddressLoadRule.cpp`
- Two match orders in one rule -> `src/Rules/BitExtractRule.cpp`
- Cross-window dataflow constraint via matcher reuse -> `src/Rules/ShiftMaskRule.cpp`
- Homogeneous variant sweep (tuple table) -> `src/Rules/LoadZeroExtendRule.cpp`

Class skeleton (every rule follows this; see any exemplar for the full file):

```cpp
class XRule final : public Rule {
  public:
    llvm::StringRef getID() const override;            // "category/name"
    llvm::StringRef getDescription() const override;   // imperative: "fold X into Y"
    unsigned getInstructionCount() const override;     // window size this rule matches
    bool shouldRun(const Context &Ctx) const override; // only when whole-rule gating needed
    std::optional<Match> match(llvm::ArrayRef<Instruction> Instructions,
                               const Context &Ctx) const override;
};
```

### 2.2. Requirements

- Architecture gating: if the entire rule is variant-specific, override `shouldRun` (see `NopLA64Rule`); if only some arms are, check `Ctx.Arch` inside `match()` (see `ShiftMaskRule`). On LA32, "LA32" means LA32S: LA32S-granted instructions (`BSTRPICK.W`, `ANDN`/`ORN`, `BEQZ`/`BNEZ`, byte/bit operations) are available without extra gating, and `.D`/64-only forms are not. Note that some instructions might have different implications on LA32 and LA64.
  - For example, `ADDI.W Rd, Rd, 0` is an no-op on LA32 since the register is one-word wide. It is a sign-extension from word to double-word on LA64, however.
- Return `std::nullopt` for no finding. A `Match` replaces the whole window in the suggestion; deletion rules return the surviving instruction (`Result.Replacement.push_back(F)`), fusion rules build new instructions with `MCInstBuilder`.
- Style: `.clang-format` (LLVM-based, 4-space indent, column 100). PascalCase everywhere, `The` prefix for global singletons, no multiple definitions-with-initializers on one line. Mnemonics in our own comments and strings are UPPERCASE (`ADD.D`); never rewrite LLVM's disassembly output -- fixtures contain real, lowercase assembly exactly as llvm-mc accepts it.
- Documentation: do not hard-wrap Markdown anywhere; let the renderer lay it out.

### 2.3. When writing Rule files

#### 2.3.1. Building replacements

`Rule::Match::Replacement` is a `llvm::SmallVector<llvm::MCInst, 0>` that replaces the entire matched window in the suggestion.

- Build modified instructions with `MCInstBuilder`: `MCInstBuilder(Op).addReg(Rd).addReg(Rj).addImm(Combined)`. Operand order must follow the canonical TableGen definition.
- Keep an instruction verbatim with `Result.Replacement.push_back(F);` (deletion of its partner).
- Immediate fields that TableGen defines as unsigned bitfields (e.g. `BSTRPICK`'s msb/lsb) still travel through `Imm`'s `int64_t` -- the values are small positives, but cast deliberately when deriving them from arithmetic (`static_cast<unsigned>(Lsb)`).
- Recombining offsets needs the encoder's rules: plain load/store offsets are `isInt<12>`, `LDPTR`'s decoded immediate is already scaled, so a combined value is checked with `isShiftedInt<14, 2>` (see `AddressLoadRule`).

#### 2.3.2. Window mechanics

`RuleManager::runWindow` hands each rule exactly `getInstructionCount()` consecutive `Instruction` records; `Instructions[i].Inst` is the `MCInst`. Assert the size at the top of `match()` (`assert(Instructions.size() == 2 && "...")`). Windows are adjacent and slide by one instruction, so a 2-instruction rule sees every consecutive pair once per scan position. Rules whose applicability depends only on the target variant should say so via `shouldRun` instead of testing `Ctx.Arch` in every `match()` call -- the manager then skips them entirely.

## 3. Soundness rules (peephole-wide liveness guarantee)

LoongLint is a peephole linter. If a Rule cannot prove, via the pattern itself, that the liveness requirement is satisfied within the visible peephole, reject.

- Only same-destination rewrites. Deleting an instruction requires proving it dead, which a 2-instruction window cannot do -- so fusion rules must overwrite the temporary with the replacement's own destination.
- Be careful with architecturally-wired `$zero`. This register ignores all writes to it and always reads back 0. This means that some fusions cannot be safe with `$zero`, e.g.:
  - Never route memory addresses through `$zero` as the intermediate: with `addi.d $zero, ...` + `ld.* $zero, $zero, 0` the original load reads `0 + disp` while the replacement reads `rj + disp` -- a different address that can fault. See `AddressLoadRule`'s `R0` rejection.
- Reject aliasing that changes semantics (e.g. the shift-count register doubling as the shifted-value register in `ShiftMaskRule`).
- When uncertain, reject. False negatives are cheap; false positives burn user trust.

## 4. Gotchas

- Matcher captures persist after a successful `matchInst` and act as equality constraints when the same matcher object is reused; failed matches roll back. Reuse bound matchers deliberately (dataflow constraints), and use fresh matchers for independent attempts.
- Decoded immediates can surprise: `LDPTR` offsets arrive already scaled (`isShiftedInt<14,2>` on the combined value), and `ALSL`'s assembly immediate is the real shift amount 1..4 (encoded as one less), etc. Always confirm the configured LLVM source and the "LoongArch Reference Manual - Volume 1" when in doubt.
- A mismatch-fixture "negative" for your rule can accidentally match a different rule and break the zero-findings assertion. Check each negative against every overlapping rule before committing to it.
- Keep the four orderings consistent: `RuleManager.cpp` includes and the `CMakeLists.txt` source list are alphabetical; registration order drives finding order and RULES.md section order.
- Probe operand layouts with `llvm-mc` before writing fixtures -- assemble the exact sequence you intend to match.

## 5. Build and test

Configure once (see the README "Installation" section for the minimal variant; a debug configure with assertions gives the best diagnostics):

```sh
cmake -S "$LLVM_PROJECT"/llvm -B build -G Ninja \
  -DLLVM_EXTERNAL_PROJECTS='loonglint' \
  -DLLVM_EXTERNAL_LOONGLINT_SOURCE_DIR="$(pwd)" \
  -DLLVM_TARGETS_TO_BUILD='LoongArch' \
  -DCMAKE_BUILD_TYPE=Debug \
  -DLLVM_ENABLE_ASSERTIONS=ON \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
```

Then, after each change:

```sh
ninja -C build loonglint
ninja -C build check-loonglint
```

`check-loonglint` runs the lit suite (fixtures) and the unit tests; lit discovers new fixture files automatically (i.e. no CMake edit needed for tests). If the working copy has no changes, do not bother rebuilding.
