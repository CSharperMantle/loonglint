// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/Rules/LoongArch/ZeroCompareBranchRule.hpp"

#include "loonglint/MCInstMatcher.hpp"

#include "MCTargetDesc/LoongArchMCTargetDesc.h"

#include "llvm/MC/MCInstBuilder.h"

#include <cassert>
#include <cstdint>

using namespace llvm;

namespace loonglint::LoongArch {

namespace LoongArch = ::llvm::LoongArch;

StringRef ZeroCompareBranchRule::getID() const {
    return "loongarch:control/zero-compare-branch";
}

StringRef ZeroCompareBranchRule::getDescription() const {
    return "rewrite compare-against-zero branch into BEQZ/BNEZ";
}

unsigned ZeroCompareBranchRule::getInstructionCount() const {
    return 1;
}

std::optional<Rule::Match> ZeroCompareBranchRule::match(ArrayRef<Instruction> Instructions,
                                                        const Context &Ctx) const {
    using namespace LowLevelInstMatcherDSL;

    assert(Instructions.size() == 1 && "zero-compare-branch rule requires one instruction");

    const Instruction &DI = Instructions.front();
    const MCInst &I = DI.Inst;

    if (!Ctx.MIA.isBranch(I) || Ctx.MIA.isCall(I) || Ctx.MIA.isIndirectBranch(I))
        return std::nullopt;

    const unsigned Op = I.getOpcode();
    unsigned NewOp;
    switch (Op) {
    case LoongArch::BEQ:
        NewOp = LoongArch::BEQZ;
        break;
    case LoongArch::BNE:
        NewOp = LoongArch::BNEZ;
        break;
    default:
        return std::nullopt;
    }

    Reg RjReg, RdReg;
    if (!matchInst(I, Op, RjReg, RdReg, Imm()))
        return std::nullopt;

    MCRegister BranchReg;
    if (RjReg.get() == LoongArch::R0)
        BranchReg = RdReg.get();
    else if (RdReg.get() == LoongArch::R0)
        BranchReg = RjReg.get();
    else
        return std::nullopt;

    if (BranchReg == LoongArch::R0)
        // Covered by DegenerateBranchRule.
        return std::nullopt;

    uint64_t Target = 0;
    if (!Ctx.MIA.evaluateBranch(I, DI.Address, 4, Target))
        return std::nullopt;

    if (Target == DI.Address + 4)
        // Covered by BranchToNextRule.
        return std::nullopt;

    Rule::Match Result;
    Result.Replacement.emplace_back(
        MCInstBuilder(NewOp).addReg(BranchReg).addImm(static_cast<int64_t>(Target - DI.Address)));
    return Result;
}

} // namespace loonglint::LoongArch
