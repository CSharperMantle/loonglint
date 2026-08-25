// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/Rules/NotOrRule.hpp"

#include "loonglint/MCInstMatcher.hpp"

#include "MCTargetDesc/LoongArchMCTargetDesc.h"

#include "llvm/MC/MCInstBuilder.h"

#include <cassert>

using namespace llvm;

namespace loonglint {

StringRef NotOrRule::getID() const {
    return "integer/not-or";
}

StringRef NotOrRule::getDescription() const {
    return "fold OR; NOT into NOR";
}

unsigned NotOrRule::getInstructionCount() const {
    return 2;
}

std::optional<Rule::Match> NotOrRule::match(ArrayRef<Instruction> Instructions,
                                            const Context &) const {
    using namespace LowLevelInstMatcherDSL;

    assert(Instructions.size() == 2 && "integer/not-or requires two instructions");

    Reg OrRdReg, OrRjReg, OrRkReg;
    if (!matchInst(Instructions[0].Inst, LoongArch::OR, OrRdReg, OrRjReg, OrRkReg))
        return std::nullopt;
    const MCRegister OrRd = OrRdReg.get();
    const MCRegister OrRj = OrRjReg.get();
    const MCRegister OrRk = OrRkReg.get();

    // NOR OrRd, OrRd, $zero | NOR OrRd, $zero, OrRd: one operand is the OR result, the other is
    // $zero.
    Reg NorRjReg, NorRkReg;
    if (!matchInst(Instructions[1].Inst, LoongArch::NOR, OrRdReg, NorRjReg, NorRkReg))
        return std::nullopt;
    const MCRegister NorRj = NorRjReg.get();
    const MCRegister NorRk = NorRkReg.get();

    if ((NorRj == OrRd) == (NorRk == OrRd))
        return std::nullopt;
    const MCRegister Other = (NorRj == OrRd) ? NorRk : NorRj;
    if (Other != LoongArch::R0)
        return std::nullopt;

    Rule::Match Result;
    Result.Replacement.emplace_back(
        MCInstBuilder(LoongArch::NOR).addReg(OrRd).addReg(OrRj).addReg(OrRk));
    return Result;
}

} // namespace loonglint
