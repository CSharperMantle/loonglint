// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/Rules/AndNotRule.hpp"

#include "loonglint/MCInstMatcher.hpp"

#include "MCTargetDesc/LoongArchMCTargetDesc.h"

#include "llvm/MC/MCInstBuilder.h"

#include <cassert>

using namespace llvm;

namespace loonglint {

StringRef AndNotRule::getID() const {
    return "integer/and-not";
}

StringRef AndNotRule::getDescription() const {
    return "fold NOR; AND into ANDN";
}

unsigned AndNotRule::getInstructionCount() const {
    return 2;
}

std::optional<Rule::Match> AndNotRule::match(ArrayRef<Instruction> Instructions,
                                             const Context &) const {
    using namespace LowLevelInstMatcherDSL;

    assert(Instructions.size() == 2 && "integer/and-not requires two instructions");

    Reg NorRdReg, NorRjReg, NorRkReg;
    if (!matchInst(Instructions[0].Inst, LoongArch::NOR, NorRdReg, NorRjReg, NorRkReg))
        return std::nullopt;
    const MCRegister NorRd = NorRdReg.get();
    const MCRegister NorRj = NorRjReg.get();
    const MCRegister NorRk = NorRkReg.get();

    if ((NorRj == LoongArch::R0) == (NorRk == LoongArch::R0)) // exactly one operand is $zero
        return std::nullopt;

    const MCRegister AndnRk = (NorRj == LoongArch::R0) ? NorRk : NorRj;
    if (AndnRk == NorRdReg.get())
        return std::nullopt;

    Reg AndRjReg, AndRkReg;
    if (!matchInst(Instructions[1].Inst, LoongArch::AND, NorRdReg, AndRjReg, AndRkReg))
        return std::nullopt;
    const MCRegister AndRj = AndRjReg.get();
    const MCRegister AndRk = AndRkReg.get();

    if ((AndRj == NorRd) == (AndRk == NorRd))
        return std::nullopt;

    const MCRegister AndnRj = (AndRj == NorRd) ? AndRk : AndRj;
    if (AndnRj == NorRd)
        return std::nullopt;

    Rule::Match Result;
    Result.Replacement.emplace_back(
        MCInstBuilder(LoongArch::ANDN).addReg(NorRd).addReg(AndnRj).addReg(AndnRk));
    return Result;
}

} // namespace loonglint
