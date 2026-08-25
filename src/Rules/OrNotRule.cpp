// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/Rules/OrNotRule.hpp"

#include "loonglint/MCInstMatcher.hpp"

#include "MCTargetDesc/LoongArchMCTargetDesc.h"

#include "llvm/MC/MCInstBuilder.h"

#include <cassert>

using namespace llvm;

namespace loonglint {

StringRef OrNotRule::getID() const {
    return "integer/or-not";
}

StringRef OrNotRule::getDescription() const {
    return "fold NOR; OR into ORN";
}

unsigned OrNotRule::getInstructionCount() const {
    return 2;
}

std::optional<Rule::Match> OrNotRule::match(ArrayRef<Instruction> Instructions,
                                            const Context &) const {
    using namespace LowLevelInstMatcherDSL;

    assert(Instructions.size() == 2 && "integer/or-not requires two instructions");

    Reg NorRdReg, NorRjReg, NorRkReg;
    if (!matchInst(Instructions[0].Inst, LoongArch::NOR, NorRdReg, NorRjReg, NorRkReg))
        return std::nullopt;
    const MCRegister NorRd = NorRdReg.get();
    const MCRegister NorRj = NorRjReg.get();
    const MCRegister NorRk = NorRkReg.get();

    if ((NorRj == LoongArch::R0) == (NorRk == LoongArch::R0)) // exactly one operand is $zero
        return std::nullopt;

    const MCRegister OrnRk = (NorRj == LoongArch::R0) ? NorRk : NorRj;
    if (OrnRk == NorRd)
        return std::nullopt;

    Reg OrRjReg, OrRkReg;
    if (!matchInst(Instructions[1].Inst, LoongArch::OR, NorRdReg, OrRjReg, OrRkReg))
        return std::nullopt;
    const MCRegister OrRj = OrRjReg.get();
    const MCRegister OrRk = OrRkReg.get();

    if ((OrRj == NorRd) == (OrRk == NorRd))
        return std::nullopt;

    const MCRegister OrnRj = (OrRj == NorRd) ? OrRk : OrRj;
    if (OrnRj == NorRd)
        return std::nullopt;

    Rule::Match Result;
    Result.Replacement.emplace_back(
        MCInstBuilder(LoongArch::ORN).addReg(NorRd).addReg(OrnRj).addReg(OrnRk));
    return Result;
}

} // namespace loonglint
