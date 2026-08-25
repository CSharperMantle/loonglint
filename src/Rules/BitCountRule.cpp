// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/Rules/BitCountRule.hpp"

#include "loonglint/MCInstMatcher.hpp"

#include "MCTargetDesc/LoongArchMCTargetDesc.h"

#include "llvm/MC/MCInstBuilder.h"
#include "llvm/MC/MCRegister.h"

#include <cassert>

using namespace llvm;

namespace loonglint {

StringRef BitCountRule::getID() const {
    return "integer/bit-count";
}

StringRef BitCountRule::getDescription() const {
    return "fold complemented bit count";
}

unsigned BitCountRule::getInstructionCount() const {
    return 2;
}

std::optional<Rule::Match> BitCountRule::match(ArrayRef<Instruction> Instructions,
                                               const Context &) const {
    using namespace LowLevelInstMatcherDSL;

    assert(Instructions.size() == 2 && "integer/bit-count requires two instructions");

    Reg NorRdReg, NorRjReg, NorRkReg;
    if (!matchInst(Instructions[0].Inst, LoongArch::NOR, NorRdReg, NorRjReg, NorRkReg))
        return std::nullopt;
    const MCRegister NorRd = NorRdReg.get();
    const MCRegister NorRj = NorRjReg.get();
    const MCRegister NorRk = NorRkReg.get();

    if ((NorRj == LoongArch::R0) == (NorRk == LoongArch::R0))
        return std::nullopt;
    const MCRegister Rj = (NorRj == LoongArch::R0) ? NorRk : NorRj;

    const MCInst &Second = Instructions[1].Inst;
    unsigned NewOp;
    switch (Second.getOpcode()) {
    case LoongArch::CLZ_W:
        NewOp = LoongArch::CLO_W;
        break;
    case LoongArch::CTZ_W:
        NewOp = LoongArch::CTO_W;
        break;
    case LoongArch::CLZ_D:
        NewOp = LoongArch::CLO_D;
        break;
    case LoongArch::CTZ_D:
        NewOp = LoongArch::CTO_D;
        break;
    default:
        return std::nullopt;
    }

    if (!matchInst(Second, Second.getOpcode(), NorRdReg, NorRdReg))
        return std::nullopt;

    Rule::Match Result;
    Result.Replacement.emplace_back(MCInstBuilder(NewOp).addReg(NorRd).addReg(Rj));
    return Result;
}

} // namespace loonglint
