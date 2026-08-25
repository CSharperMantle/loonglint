// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/Rules/ShiftChainRule.hpp"

#include "loonglint/MCInstMatcher.hpp"

#include "MCTargetDesc/LoongArchMCTargetDesc.h"

#include "llvm/MC/MCInstBuilder.h"

#include <algorithm>
#include <cassert>
#include <cstdint>

using namespace llvm;

namespace loonglint {

StringRef ShiftChainRule::getID() const {
    return "integer/shift-chain";
}

StringRef ShiftChainRule::getDescription() const {
    return "fuse adjacent same-direction shifts";
}

unsigned ShiftChainRule::getInstructionCount() const {
    return 2;
}

std::optional<Rule::Match> ShiftChainRule::match(ArrayRef<Instruction> Instructions,
                                                 const Context &) const {
    using namespace LowLevelInstMatcherDSL;

    assert(Instructions.size() == 2 && "integer/shift-chain requires two instructions");

    const MCInst &F = Instructions[0].Inst;
    const MCInst &S = Instructions[1].Inst;
    const unsigned Op = F.getOpcode();

    bool IsD = false;
    bool IsArith = false;
    switch (Op) {
    case LoongArch::SLLI_D:
    case LoongArch::SRLI_D:
    case LoongArch::SRAI_D:
        IsD = true;
        IsArith = Op == LoongArch::SRAI_D;
        break;
    case LoongArch::SLLI_W:
    case LoongArch::SRLI_W:
    case LoongArch::SRAI_W:
        IsArith = Op == LoongArch::SRAI_W;
        break;
    default:
        return std::nullopt;
    }

    Reg RdReg, RjReg;
    Imm FirstShamtImm;
    if (!matchInst(F, Op, RdReg, RjReg, FirstShamtImm))
        return std::nullopt;
    const MCRegister Rd = RdReg.get();
    const MCRegister Rj = RjReg.get();
    const int64_t FirstShamt = FirstShamtImm.get();

    // Second shift reuses the temporary: op rd, rd, Shamt.
    Imm SecondShamtImm;
    if (!matchInst(S, Op, RdReg, RdReg, SecondShamtImm))
        return std::nullopt;
    const int64_t SecondShamt = SecondShamtImm.get();

    if (FirstShamt < 1 || SecondShamt < 1)
        return std::nullopt;

    const int64_t MaxShift = IsD ? 63 : 31;
    int64_t Combined;
    if (IsArith)
        Combined = std::min(FirstShamt + SecondShamt, MaxShift);
    else {
        if (FirstShamt + SecondShamt > MaxShift)
            return std::nullopt;
        Combined = FirstShamt + SecondShamt;
    }

    Rule::Match Result;
    Result.Replacement.emplace_back(MCInstBuilder(Op).addReg(Rd).addReg(Rj).addImm(Combined));
    return Result;
}

} // namespace loonglint
