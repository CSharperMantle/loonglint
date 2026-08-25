// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/Rules/ByteReverseRule.hpp"

#include "loonglint/MCInstMatcher.hpp"

#include "MCTargetDesc/LoongArchMCTargetDesc.h"

#include "llvm/MC/MCInstBuilder.h"

#include <cassert>

using namespace llvm;

namespace loonglint {

StringRef ByteReverseRule::getID() const {
    return "integer/byte-reverse";
}

StringRef ByteReverseRule::getDescription() const {
    return "fold halfword/byte reversal into REVB.D";
}

unsigned ByteReverseRule::getInstructionCount() const {
    return 2;
}

bool ByteReverseRule::shouldRun(const Context &Ctx) const {
    return Ctx.Arch == Architecture::LoongArch64;
}

std::optional<Rule::Match> ByteReverseRule::match(ArrayRef<Instruction> Instructions,
                                                  const Context &) const {
    using namespace LowLevelInstMatcherDSL;

    assert(Instructions.size() == 2 && "integer/byte-reverse requires two instructions");

    const MCInst &F = Instructions[0].Inst;
    const MCInst &S = Instructions[1].Inst;

    for (const auto &[FirstOp, SecondOp] : {
             std::make_tuple(LoongArch::REVB_4H, LoongArch::REVH_D),
             std::make_tuple(LoongArch::REVH_D, LoongArch::REVB_4H),
         }) {
        Reg RdReg, RjReg;
        if (!matchInst(F, FirstOp, RdReg, RjReg))
            continue;
        const MCRegister Rd = RdReg.get(), Rj = RjReg.get();
        if (!matchInst(S, SecondOp, RdReg, RdReg))
            continue;
        Rule::Match Result;
        Result.Replacement.emplace_back(MCInstBuilder(LoongArch::REVB_D).addReg(Rd).addReg(Rj));
        return Result;
    }

    return std::nullopt;
}

} // namespace loonglint
