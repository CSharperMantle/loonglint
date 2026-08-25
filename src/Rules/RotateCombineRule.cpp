// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/Rules/RotateCombineRule.hpp"

#include "loonglint/MCInstMatcher.hpp"

#include "MCTargetDesc/LoongArchMCTargetDesc.h"

#include "llvm/MC/MCInstBuilder.h"

#include <cassert>
#include <cstdint>

using namespace llvm;

namespace loonglint {

StringRef RotateCombineRule::getID() const {
    return "integer/rotate-combine";
}

StringRef RotateCombineRule::getDescription() const {
    return "fuse adjacent rotations";
}

unsigned RotateCombineRule::getInstructionCount() const {
    return 2;
}

std::optional<Rule::Match> RotateCombineRule::match(ArrayRef<Instruction> Instructions,
                                                    const Context &) const {
    using namespace LowLevelInstMatcherDSL;

    assert(Instructions.size() == 2 && "integer/rotate-combine requires two instructions");

    const MCInst &F = Instructions[0].Inst;
    const MCInst &S = Instructions[1].Inst;
    const unsigned Op = F.getOpcode();

    bool IsD = false;
    if (Op == LoongArch::ROTRI_D)
        IsD = true;
    else if (Op == LoongArch::ROTRI_W)
        IsD = false;
    else
        return std::nullopt;

    Reg RdReg, RjReg;
    Imm FirstShamtImm;
    if (!matchInst(F, Op, RdReg, RjReg, FirstShamtImm))
        return std::nullopt;
    const MCRegister Rd = RdReg.get(), Rj = RjReg.get();
    const int64_t FirstShamt = FirstShamtImm.get();

    Imm SecondShamtImm;
    if (!matchInst(S, Op, RdReg, RdReg, SecondShamtImm))
        return std::nullopt;
    const int64_t SecondShamt = SecondShamtImm.get();

    if (FirstShamt < 1 || SecondShamt < 1)
        return std::nullopt;

    const int64_t Width = IsD ? 64 : 32;
    const int64_t Combined = (FirstShamt + SecondShamt) % Width;

    Rule::Match Result;
    Result.Replacement.emplace_back(MCInstBuilder(Op).addReg(Rd).addReg(Rj).addImm(Combined));
    return Result;
}

} // namespace loonglint
