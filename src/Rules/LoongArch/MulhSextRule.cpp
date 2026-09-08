// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/Rules/LoongArch/MulhSextRule.hpp"

#include "loonglint/LoongArch/LoongArchSpec.hpp"
#include "loonglint/MCInstMatcher.hpp"

#include "MCTargetDesc/LoongArchMCTargetDesc.h"

#include <cassert>

using namespace llvm;

namespace loonglint::LoongArch {

namespace LoongArch = ::llvm::LoongArch;

StringRef MulhSextRule::getID() const {
    return "loongarch:integer/mulh-sext";
}

StringRef MulhSextRule::getDescription() const {
    return "delete redundant high-multiply sign extension";
}

unsigned MulhSextRule::getInstructionCount() const {
    return 2;
}

MulhSextRule::MulhSextRule(const LoongArchSpec &LoongAS) : LoongAS(LoongAS) {}

bool MulhSextRule::shouldRun(const Context &Ctx) const {
    return LoongAS.is64();
}

std::optional<Rule::Match> MulhSextRule::match(ArrayRef<Instruction> Instructions,
                                               const Context &) const {
    using namespace LowLevelInstMatcherDSL;

    assert(Instructions.size() == 2 && "integer/mulh-sext requires two instructions");

    const MCInst &F = Instructions[0].Inst;
    const MCInst &S = Instructions[1].Inst;

    // MULH.W/MULH.WU rd, rj, rk ; ADDI.W/SLLI.W rd, rd, 0  ->  delete the
    // extension. On LA64 both high multiplies already sign-extend their 32-bit
    // result, so either word sign-extension idiom after them is redundant.
    for (const unsigned MulhOp : {LoongArch::MULH_W, LoongArch::MULH_WU}) {
        for (const unsigned ExtOp : {LoongArch::ADDI_W, LoongArch::SLLI_W}) {
            Reg MulhRdReg;
            if (matchInst(F, MulhOp, MulhRdReg, Reg(), Reg()) &&
                matchInst(S, ExtOp, MulhRdReg, MulhRdReg, Imm(0))) {
                Rule::Match Result;
                Result.Replacement.emplace_back(F);
                return Result;
            }
        }
    }

    return std::nullopt;
}

} // namespace loonglint::LoongArch
