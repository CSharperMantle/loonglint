# The low-level instruction matcher DSL (`LowLevelInstMatcherDSL`)

The DSL lives in `include/loonglint/MCInstMatcher.hpp`. It is a derived work of LLVM BOLT's `LowLevelInstMatcherDSL` (see the header comment for provenance). It matches a single `llvm::MCInst` against an opcode plus a per-operand matcher list, and captures operands into reusable matcher objects.

Reference material when unsure about an instruction: the canonical operand order and immediate encodings are defined in `LoongArchInstrInfo.td` in the LLVM source tree; opcode enum names (`LoongArch::LD_BU`, `LoongArch::BSTRPICK_D`, ...) are generated into `LoongArchGenInstrInfo.inc` in the build tree. Include `"MCTargetDesc/LoongArchMCTargetDesc.h"` (already on the include path via the tool target) to use them.

## 1. Core semantics

A matcher that captures all three operands of the `LD.D rd, rj, si12` is as follows:

```cpp
// Define matcher objects.
Reg RdReg, RjReg;
Imm Si12Imm;
// Perform the match.
if (matchInst(Inst, LoongArch::LD_D, RdReg, RjReg, Si12Imm)) {
    // |Inst| matches the pattern. Now fetching the matched operands.
    const MCRegister Rd = RdReg.get();
    const MCRegister Rj = RjReg.get();
    const int64_t Si12 = Si12Imm.get();
    // Use the captured value.
    ...
}
```

- `matchInst(Inst, Opcode, matchers...)` fails unless the instance's opcode equals `Opcode` **and** `Inst.getNumOperands() == sizeof...(matchers)`. Operand count is exact -- there is no variadic slack.
- Each matcher is offered the corresponding `MCOperand` in order. A matcher with no captured value binds (captures) the operand value; a matcher that already holds a value succeeds only if the operand equals it (and re-captures, keeping the same value).
- All-or-nothing with rollback: before matching, every matcher saves its state; if any operand fails, every matcher is restored to its saved state. A failed `matchInst` therefore leaves no trace. A successful one leaves all captures in place.
- Captures persist across `matchInst` calls within one `match()` invocation. This is the feature that makes cross-instruction dataflow constraints possible -- and the easiest way to write an unintended constraint if you reuse a matcher variable casually.

## 2. Matchers

* `Reg` -> Binds `llvm::MCRegister` -> Used for register operands.
* `Imm` -> Binds `int64_t` -> Used for immediate operands.
* `Skip` -> Binds nothing -> Used for skipping the current slot.

All three take an optional seed that pre-binds a value, turning the matcher into a pure equality check: `Imm(0)` matches only an immediate 0, `Reg(LoongArch::R0)` only the zero register. For example, to match and capture the destination register of `addi.d rd, $zero, 1`:

```cpp
Reg RdReg;
if (matchInst(Inst, LoongArch::LD_D, RdReg, Reg(LoongArch::R0), Imm(1))) {
    ...
}
```

**Default-constructed `Reg()` / `Imm()` are typed wildcards:** They match any operand of the right kind and capture the value. Prefer them over `Skip()` when the matcher would otherwise not type-check the operand (e.g. `matchInst(Inst, LoongArch::LD_D, RdReg, Reg(), Skip())` on a load also asserts operand 1 is a register, and `..., Reg(), Imm()` asserts the second operand kind split).

**Reading captures:** `Reg::get()` / `Imm::get()` return the bound value; they assert (debug builds) that the matcher actually captured something. Capture into locals immediately after a successful match (e.g. `const MCRegister Rd = RdReg.get();`) and do the semantic checks on plain values, not on matchers.

## 3. The constraint-reuse idiom

Bind matchers on the producer instruction, then pass the same objects again on the consumer instruction to enforce dataflow:

```cpp
Reg RdReg, RjReg;
Imm FirstImm;
if (!matchInst(Inst[0], LoongArch::ADDI_W, RdReg, RjReg, FirstImm))
    return std::nullopt;

// ADDI Rd, Rd, Imm: passing RdReg twice requires both the destination and the base register to equal the first instruction's destination.
Imm SecondImm;
if (!matchInst(Inst[1], LoongArch::ADDI_W, RdReg, RdReg, SecondImm))
    return std::nullopt;

// Now we have the whole pattern matched.
...
```

If the consumer match fails, rollback restores the matchers. The pattern to avoid is reusing a bound matcher in a *later alternative attempt* where the earlier capture is not meant to constrain: either use fresh matchers per attempt (this is what per-iteration loop-local matchers in tuple-table rules do) or reset by construction. For example, the following structure can be used to trial-match multiple patterns:

```cpp
bool matches(...) {
    // Try match the first pattern.
    do {
        // Does this pattern match?
        if (!matchInst(Inst[0], ...))
            // No. Break and try the next one.
            break;
        // Yes.
        return true;
    } while (0);

    // Try match the second pattern.
    do {
        // Does this pattern match?
        if (!matchInst(Inst[0], ...))
            // No. Break and try the next one.
            break;
        // Yes.
        return true;
    } while (0);

    // Remaining patterns.
    ...

    // None of the patterns matched.
    return false;
}
```

## 4. Structural patterns, with exemplars

1. **Switch-on-opcode dispatch** (`src/Rules/ShiftMaskRule.cpp`). Classify the second instruction's opcode family first, derive width-dependent constants (`CountMask`, `MinMsb`), then match the first instruction against alternatives. Use when a family of opcodes shares one shape.
2. **Tuple-table sweep** (`src/Rules/LoadZeroExtendRule.cpp`, `src/Rules/AddiPairRule.cpp`, `src/Rules/UnsignedLoadPickRule.cpp`). Enumerate homogeneous variants as a braced initializer of tuples/pairs, loop, and `continue` on non-match. Put per-row arch flags in the table (`Needs64`) or split the loop by `Ctx.Arch` (see `UnsignedLoadPickRule`'s LA64/LA32 branches). Loop-local matchers keep attempts independent.
3. **Multi-arm sequencing** (`src/Rules/BitExtractRule.cpp`). When one rule accepts several orders (mask-first, shift-first), try each arm in a `do { ... break; ... } while (0);` block that `return`s on success and `break`s to the next arm. Each arm starts with fresh matchers.
4. **Try-helper lambda** (`src/Rules/AddressLoadRule.cpp`'s `TryLoad`). When many candidate opcodes share one match-and-build body, capture the `Rule::Match` under construction in a lambda and call it per candidate opcode; return the result on the first success.
5. **Cross-window constraint** (`src/Rules/ShiftMaskRule.cpp`). The mask instruction's destination matcher is reused in slots 1 and 3 of the shift match (`matchInst(S, Op, CountRdReg, ShRjReg, CountRdReg)`), forcing "the masked value is the shift count" in one call, followed by an explicit aliasing rejection (`ShRj == CountRd`).

## 5. Debugging a non-matching matcher

Work outward:

1. Assemble the exact sequence with `llvm-mc -triple=loongarch64-unknown-linux` and disassemble it back -- confirm the two opcodes and operand shapes you assume are what the decoder produces.
2. Check operand count first: the matcher silently returns false on count mismatch. Count operands in the `.td` definition, not in the assembly text.
3. Check for stale captures: a matcher reused from an earlier successful match now constrains instead of capturing. Give the attempt fresh matchers.
4. Check operand kinds: `Reg` fails on an immediate operand and vice versa; use `Reg()` / `Imm()` typed wildcards to surface kind mismatches.
5. If the matcher is right but the rule still rejects, instrument the semantic checks -- they are plain C++ after the matches and are the usual real culprit (`Lsb < 1`, range checks, aliasing rejections).

`unittests/MCInstMatcherTest.cpp` covers the DSL behavior.
