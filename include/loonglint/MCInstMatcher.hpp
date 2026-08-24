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

template <typename T> class OpMatcher {
    mutable std::optional<T> Value;
    mutable std::optional<T> SavedValue;

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
        const bool MatchResult = !Value || *Value == OpValue;
        Value = OpValue;
        return MatchResult;
    }

  public:
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

template <class... OpMatchers>
bool matchInst(const llvm::MCInst &Inst, unsigned Opcode, const OpMatchers &...Ops) {
    if (Inst.getOpcode() != Opcode || Inst.getNumOperands() != sizeof...(Ops))
        return false;

    (Ops.remember(), ...);

    const llvm::MCOperand *It = Inst.begin();
    const bool AllMatched = (Ops.matches(*(It++)) && ... && true);
    if (!AllMatched)
        (Ops.restore(), ...);
    return AllMatched;
}

} // namespace LowLevelInstMatcherDSL

} // namespace loonglint

#endif
