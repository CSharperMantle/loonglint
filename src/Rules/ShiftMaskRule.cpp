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
                                                const Context &) const {
    using namespace LowLevelInstMatcherDSL;

    assert(Instructions.size() == 2 && "integer/shift-mask requires two instructions");

    const MCInst &F = Instructions[0].Inst;
    const MCInst &S = Instructions[1].Inst;

    // ANDI AndiRd, AndiRj, Mask : AndiRd = AndiRj & Mask.
    Reg AndiRdReg, AndiRjReg;
    Imm MaskImm;
    if (!matchInst(F, LoongArch::ANDI, AndiRdReg, AndiRjReg, MaskImm))
        return std::nullopt;
    const MCRegister AndiRd = AndiRdReg.get();
    const MCRegister AndiRj = AndiRjReg.get();
    const int64_t Mask = MaskImm.get();

    const unsigned Op = S.getOpcode();
    bool IsD = false;
    switch (Op) {
    case LoongArch::SLL_W:
    case LoongArch::SRL_W:
    case LoongArch::SRA_W:
        IsD = false;
        break;
    case LoongArch::SLL_D:
    case LoongArch::SRL_D:
    case LoongArch::SRA_D:
        IsD = true;
        break;
    default:
        return std::nullopt;
    }

    if (Mask != (IsD ? 63 : 31))
        return std::nullopt;

    // shift AndiRd, ShRj, AndiRd : the mask result is the shift count.
    Reg ShRjReg;
    if (!matchInst(S, Op, AndiRdReg, ShRjReg, AndiRdReg))
        return std::nullopt;
    const MCRegister ShRj = ShRjReg.get();
    if (ShRj == AndiRd)
        return std::nullopt;

    Rule::Match Result;
    Result.Replacement.emplace_back(MCInstBuilder(Op).addReg(AndiRd).addReg(ShRj).addReg(AndiRj));
    return Result;
}

} // namespace loonglint
