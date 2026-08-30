// Adapted from LLVM BOLT's LowLevelInstMatcherDSL.
//
// https://github.com/llvm/llvm-project/blob/3c6519006af45003813933136037c376a23a1338/bolt/include/bolt/Core/MCInstUtils.h#L234
// https://github.com/CSharperMantle/llvm-project-bolt/blob/b96e07fd3dedb4f4b5c4aecba8bec634abc8f51a/bolt/include/bolt/Core/MCInstUtils.h#L234
//
// Originally licensed under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
//
// This Derived Work is licensed under GPL-3.0-or-later by Rong Bao.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LOONGLINT_MCINSTMATCHER_HPP
#define LOONGLINT_MCINSTMATCHER_HPP

#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCRegister.h"

#include <cassert>
#include <cstdint>
#include <optional>

namespace loonglint {

namespace LowLevelInstMatcherDSL {

/// The base class to match an operand of type T.
///
/// The subclasses of OpMatcher are intended to be allocated on the stack and
/// to only be used by passing them to matchInst() and by calling their get()
/// function, thus the peculiar `mutable` specifiers: to make the calling code
/// compact and readable, the templated matchInst() function has to accept both
/// long-lived Imm/Reg wrappers declared as local variables (intended to capture
/// the first operand's value and match the subsequent operands, whether inside
/// a single instruction or across multiple instructions), as well as temporary
/// wrappers around literal values to match, f.e. Imm(42) or Reg(AArch64::XZR).
template <typename T> class OpMatcher {
    mutable std::optional<T> Value;
    mutable std::optional<T> SavedValue;

    // Remember/restore the last Value - to be called by matchInst.
    void remember() const {
        SavedValue = Value;
    }
    void restore() const {
        Value = SavedValue;
    }

    template <class... OpMatchers>
    friend bool matchInst(const llvm::MCInst &, unsigned, const OpMatchers &...);

  protected:
    explicit OpMatcher(std::optional<T> ValueToMatch) : Value(ValueToMatch) {}

    bool matchValue(T OpValue) const {
        // Check that OpValue does not contradict the existing Value.
        const bool MatchResult = !Value || *Value == OpValue;
        // If MatchResult is false, all matchers will be reset before returning from
        // matchInst, including this one, thus no need to assign conditionally.
        Value = OpValue;
        return MatchResult;
    }

  public:
    /// Returns the captured value.
    T get() const {
        assert(Value && "operand matcher has no captured value");
        return *Value;
    }
};

class Reg : public OpMatcher<llvm::MCRegister> {
    bool matches(const llvm::MCOperand &Op) const {
        return Op.isReg() && matchValue(Op.getReg());
    }

    template <class... OpMatchers>
    friend bool matchInst(const llvm::MCInst &, unsigned, const OpMatchers &...);

  public:
    explicit Reg(std::optional<llvm::MCRegister> RegToMatch = std::nullopt)
        : OpMatcher(RegToMatch) {}
};

class Imm : public OpMatcher<int64_t> {
    bool matches(const llvm::MCOperand &Op) const {
        return Op.isImm() && matchValue(Op.getImm());
    }

    template <class... OpMatchers>
    friend bool matchInst(const llvm::MCInst &, unsigned, const OpMatchers &...);

  public:
    explicit Imm(std::optional<int64_t> ImmToMatch = std::nullopt) : OpMatcher(ImmToMatch) {}
};

class Skip {
    void remember() const {}
    void restore() const {}
    bool matches(const llvm::MCOperand &) const {
        return true;
    }

    template <class... OpMatchers>
    friend bool matchInst(const llvm::MCInst &, unsigned, const OpMatchers &...);
};

/// Tries to match Inst and updates Ops on success.
///
/// If Inst has the specified Opcode and its operand list prefix matches Ops,
/// this function returns true and updates Ops, otherwise false is returned and
/// values of Ops are kept as before matchInst was called.
///
/// Passing std::nullopt to Opcode skips this initial check and can serve as a
/// wildcard extractor.
///
/// Please note that while Ops are technically passed by a const reference to
/// make invocations like `matchInst(MI, Opcode, Imm(42))` possible, all their
/// fields are marked mutable.
template <class... OpMatchers>
bool matchInst(const llvm::MCInst &Inst, unsigned Opcode, const OpMatchers &...Ops) {
    if (Inst.getOpcode() != Opcode || sizeof...(Ops) > Inst.getNumOperands())
        return false;

    // Ask each matcher to remember its current value in case of rollback.
    (Ops.remember(), ...);

    // Check if all matchers match the corresponding operands.
    const llvm::MCOperand *It = Inst.begin();
    const bool AllMatched = (Ops.matches(*(It++)) && ... && true);

    // If match failed, restore the original captured values.
    if (!AllMatched)
        (Ops.restore(), ...);

    return AllMatched;
}

} // namespace LowLevelInstMatcherDSL

} // namespace loonglint

#endif
