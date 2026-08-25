// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/Rules/AddiPairRule.hpp"

#include "loonglint/MCInstMatcher.hpp"

#include "MCTargetDesc/LoongArchMCTargetDesc.h"

#include "llvm/MC/MCInstBuilder.h"
#include "llvm/Support/MathExtras.h"

#include <cassert>
#include <cstdint>

using namespace llvm;

namespace loonglint {

StringRef AddiPairRule::getID() const {
    return "integer/addi-pair";
}

StringRef AddiPairRule::getDescription() const {
    return "fuse adjacent ADDI.[WD] pair";
}

unsigned AddiPairRule::getInstructionCount() const {
    return 2;
}

std::optional<Rule::Match> AddiPairRule::match(ArrayRef<Instruction> Instructions,
                                               const Context &) const {
    using namespace LowLevelInstMatcherDSL;

    assert(Instructions.size() == 2 && "integer/addi-pair requires two instructions");

    const MCInst &F = Instructions[0].Inst;
    const MCInst &S = Instructions[1].Inst;

    for (const auto &[Op, IsD] : {
             std::make_tuple(LoongArch::ADDI_W, false),
             std::make_tuple(LoongArch::ADDI_D, true),
         }) {
        Reg RdReg, RjReg;
        Imm FirstSi12Imm;
        if (!matchInst(F, Op, RdReg, RjReg, FirstSi12Imm))
            continue;
        const MCRegister Rd = RdReg.get();
        const MCRegister Rj = RjReg.get();
        const int64_t FirstSi12 = FirstSi12Imm.get();

        Imm SecondSi12Imm;
        // ADDI rd, rd, Si12: second overwrites the temporary.
        if (!matchInst(S, Op, RdReg, RdReg, SecondSi12Imm))
            continue;
        const int64_t SecondSi12 = SecondSi12Imm.get();

        if (FirstSi12 == 0 || SecondSi12 == 0) // an ADDI-by-0 is an identity, not a pair member
            continue;
        const int64_t Combined = FirstSi12 + SecondSi12;
        if (!isInt<12>(Combined))
            continue;

        Rule::Match Result;
        Result.Replacement.emplace_back(MCInstBuilder(Op).addReg(Rd).addReg(Rj).addImm(Combined));
        return Result;
    }

    return std::nullopt;
}

} // namespace loonglint
