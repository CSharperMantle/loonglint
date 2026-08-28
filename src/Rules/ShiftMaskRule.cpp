// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/Rules/ShiftMaskRule.hpp"

#include "loonglint/MCInstMatcher.hpp"

#include "MCTargetDesc/LoongArchMCTargetDesc.h"

#include "llvm/MC/MCInstBuilder.h"

#include <cassert>
#include <cstdint>

using namespace llvm;

namespace loonglint {

StringRef ShiftMaskRule::getID() const {
    return "integer/shift-mask";
}

StringRef ShiftMaskRule::getDescription() const {
    return "delete redundant shift-amount mask";
}

unsigned ShiftMaskRule::getInstructionCount() const {
    return 2;
}

std::optional<Rule::Match> ShiftMaskRule::match(ArrayRef<Instruction> Instructions,
                                                const Context &Ctx) const {
    using namespace LowLevelInstMatcherDSL;

    assert(Instructions.size() == 2 && "integer/shift-mask requires two instructions");

    const MCInst &F = Instructions[0].Inst;
    const MCInst &S = Instructions[1].Inst;

    // Variable shifts consume only the low 5 (.W) or 6 (.D) count bits.
    const unsigned Op = S.getOpcode();
    bool IsD = false;
    switch (Op) {
    case LoongArch::SLL_W:
    case LoongArch::SRL_W:
    case LoongArch::SRA_W:
    case LoongArch::ROTR_W:
        IsD = false;
        break;
    case LoongArch::SLL_D:
    case LoongArch::SRL_D:
    case LoongArch::SRA_D:
    case LoongArch::ROTR_D:
        IsD = true;
        break;
    default:
        return std::nullopt;
    }
    const int64_t CountMask = IsD ? 63 : 31;

    // The count producer is either an ANDI keeping every count bit, or, on
    // LA64, a BSTRPICK.D with lsb 0 keeping at least the count bits.
    Reg CountRdReg, CountRjReg;
    Imm MaskImm;
    if (matchInst(F, LoongArch::ANDI, CountRdReg, CountRjReg, MaskImm)) {
        if ((MaskImm.get() & CountMask) != CountMask)
            return std::nullopt;
    } else if (Ctx.Arch == Architecture::LoongArch64) {
        Imm MsbImm;
        if (!matchInst(F, LoongArch::BSTRPICK_D, CountRdReg, CountRjReg, MsbImm, Imm(0)))
            return std::nullopt;
        const int64_t MinMsb = IsD ? 5 : 4;
        if (MsbImm.get() < MinMsb)
            return std::nullopt;
    } else {
        return std::nullopt;
    }

    // shift CountRd, ShRj, CountRd : the masked value is the shift count.
    Reg ShRjReg;
    if (!matchInst(S, Op, CountRdReg, ShRjReg, CountRdReg))
        return std::nullopt;
    const MCRegister CountRd = CountRdReg.get();
    const MCRegister ShRj = ShRjReg.get();
    if (ShRj == CountRd)
        return std::nullopt;

    Rule::Match Result;
    Result.Replacement.emplace_back(
        MCInstBuilder(Op).addReg(CountRd).addReg(ShRj).addReg(CountRjReg.get()));
    return Result;
}

} // namespace loonglint
