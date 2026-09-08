# The low-level instruction matcher DSL (`LowLevelInstMatcherDSL`)

Lives in `include/loonglint/MCInstMatcher.hpp`; derived from LLVM BOLT's DSL (see header comment). Matches one `llvm::MCInst` against an opcode plus a per-operand matcher list, capturing operands into reusable matcher objects.

Opcode names (`LoongArch::LD_BU`, `LoongArch::BSTRPICK_D`, ...) are generated into `LoongArchGenInstrInfo.inc`; include `"MCTargetDesc/LoongArchMCTargetDesc.h"` to use them. Canonical operand order and immediate encodings live in `LoongArchInstrInfo.td` in the LLVM source tree.

**Namespace hazard:** rule cpps sit in `namespace loonglint::LoongArch`, which shadows `llvm::LoongArch`. Declare `namespace LoongArch = ::llvm::LoongArch;` inside it (every exemplar does) or `LoongArch::ADDI_D`-style references fail to compile.

## 1. Core semantics

```cpp
Reg RdReg, RjReg;
Imm Si12Imm;
if (matchInst(Inst, LoongArch::LD_D, RdReg, RjReg, Si12Imm)) {
    const MCRegister Rd = RdReg.get();
    const int64_t Si12 = Si12Imm.get();
    ...
}
```

- `matchInst(Inst, Opcode, matchers...)` fails unless the opcode equals `Opcode` **and** the operand count is exactly `sizeof...(matchers)` -- no variadic slack.
- A matcher with no captured value binds; one that already holds a value succeeds only on equality.
- All-or-nothing with rollback: any operand failure restores every matcher. A failed `matchInst` leaves no trace; a successful one leaves all captures.
- Captures persist across `matchInst` calls within one `match()` -- this enables cross-instruction dataflow constraints and is also the easiest way to write an unintended constraint.

## 2. Matchers

- `Reg` binds `llvm::MCRegister`; `Imm` binds `int64_t`; `Skip` binds nothing.
- All take an optional seed that turns them into equality checks: `Imm(0)`, `Reg(LoongArch::R0)`.
- Default-constructed `Reg()`/`Imm()` are typed wildcards: they capture and also assert the operand kind. Prefer them over `Skip()` when the kind itself should be checked.
- `get()` asserts (debug) that a capture happened; capture into locals right after the match and do semantic checks on plain values.

For example, to match and capture the destination register of `addi.d rd, $zero, 1`:

```cpp
Reg RdReg;
if (matchInst(Inst, LoongArch::LD_D, RdReg, Reg(LoongArch::R0), Imm(1))) {
    ...
}
```

## 3. Constraint reuse

Bind matchers on the producer, pass the same objects on the consumer to enforce dataflow:

```cpp
Reg RdReg, RjReg;
Imm FirstImm;
if (!matchInst(Inst[0], LoongArch::ADDI_W, RdReg, RjReg, FirstImm))
    return std::nullopt;
// Passing RdReg twice requires destination == base == the producer's destination.
Imm SecondImm;
if (!matchInst(Inst[1], LoongArch::ADDI_W, RdReg, RdReg, SecondImm))
    return std::nullopt;
```

On consumer failure, rollback restores the matchers. Do not reuse a bound matcher across *alternative attempts* where the earlier capture must not constrain: use fresh matchers per attempt (tuple-table rules use loop-local matchers), or trial-match patterns in `do { ... break; ... } while (0);` blocks that return on success.

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

## 4. Structural patterns (with exemplars)

1. **Switch-on-opcode dispatch** (`src/Rules/LoongArch/ShiftMaskRule.cpp`): classify the second instruction's opcode family, derive width-dependent constants, then match the first against alternatives.
2. **Tuple-table sweep** (`src/Rules/LoongArch/LoadZeroExtendRule.cpp`, `AddiPairRule.cpp`, `UnsignedLoadPickRule.cpp`): homogeneous variants as a braced initializer of tuples, loop, `continue` on non-match; per-row arch flags in the table or a `LoongAS.is64()` split; loop-local matchers.
3. **Multi-arm sequencing** (`src/Rules/LoongArch/BitExtractRule.cpp`): several accepted orders tried in `do { ... break; ... } while (0);` blocks; fresh matchers per arm.
4. **Try-helper lambda** (`src/Rules/LoongArch/AddressLoadRule.cpp`'s `TryLoad`): one match-and-build body invoked per candidate opcode; first success wins.
5. **Cross-window constraint** (`src/Rules/LoongArch/ShiftMaskRule.cpp`): a producer destination matcher reused in two slots of the consumer match, plus an explicit aliasing rejection.

## 5. Debugging a non-matching matcher

Work outward:

1. Assemble the exact sequence with `llvm-mc` and disassemble it back -- confirm opcodes and operand shapes.
2. Operand count first: the matcher fails silently on count mismatch. Count operands in the `.td`, not the assembly.
3. Stale captures: a matcher reused from an earlier success now constrains; give the attempt fresh matchers.
4. Operand kinds: `Reg` fails on an immediate and vice versa; typed wildcards surface kind mismatches.
5. Matcher right but rule still rejects? Instrument the semantic checks -- plain C++ after the matches is the usual real culprit.

`unittests/MCInstMatcherTest.cpp` covers the DSL behavior.
