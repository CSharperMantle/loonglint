// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/Rules/BitReverseRule.hpp"

#include "loonglint/MCInstMatcher.hpp"

#include "MCTargetDesc/LoongArchMCTargetDesc.h"

#include "llvm/MC/MCInstBuilder.h"

#include <cassert>

using namespace llvm;

namespace loonglint {

StringRef BitReverseRule::getID() const {
    return "integer/bit-reverse";
}

StringRef BitReverseRule::getDescription() const {
    return "fold byte/bit reversal into BITREV.[48]B";
}

unsigned BitReverseRule::getInstructionCount() const {
    return 2;
}

bool BitReverseRule::shouldRun(const Context &Ctx) const {
    return Ctx.Arch == Architecture::LoongArch64;
}

std::optional<Rule::Match> BitReverseRule::match(ArrayRef<Instruction> Instructions,
                                                 const Context &) const {
    using namespace LowLevelInstMatcherDSL;

    assert(Instructions.size() == 2 && "integer/bit-reverse requires two instructions");

    const MCInst &F = Instructions[0].Inst;
    const MCInst &S = Instructions[1].Inst;

    for (const auto &[FirstOp, SecondOp, ResultOp] : {
             std::make_tuple(LoongArch::REVB_2W, LoongArch::BITREV_W, LoongArch::BITREV_4B),
             std::make_tuple(LoongArch::BITREV_W, LoongArch::REVB_2W, LoongArch::BITREV_4B),
             std::make_tuple(LoongArch::REVB_D, LoongArch::BITREV_D, LoongArch::BITREV_8B),
             std::make_tuple(LoongArch::BITREV_D, LoongArch::REVB_D, LoongArch::BITREV_8B),
         }) {
        Reg RdReg, RjReg;
        if (!matchInst(F, FirstOp, RdReg, RjReg))
            continue;
        const MCRegister Rd = RdReg.get();
        const MCRegister Rj = RjReg.get();

        if (!matchInst(S, SecondOp, RdReg, RdReg))
            continue;
        Rule::Match Result;
        Result.Replacement.emplace_back(MCInstBuilder(ResultOp).addReg(Rd).addReg(Rj));
        return Result;
    }

    return std::nullopt;
}

} // namespace loonglint
