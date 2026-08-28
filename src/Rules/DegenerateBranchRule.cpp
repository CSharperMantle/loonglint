// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/Rules/DegenerateBranchRule.hpp"

#include "loonglint/MCInstMatcher.hpp"

#include "MCTargetDesc/LoongArchMCTargetDesc.h"

#include "llvm/MC/MCInstBuilder.h"

#include <cassert>
#include <cstdint>

using namespace llvm;

namespace loonglint {

StringRef DegenerateBranchRule::getID() const {
    return "control/degenerate-branch";
}

StringRef DegenerateBranchRule::getDescription() const {
    return "simplify constant branch condition";
}

unsigned DegenerateBranchRule::getInstructionCount() const {
    return 1;
}

std::optional<Rule::Match> DegenerateBranchRule::match(ArrayRef<Instruction> Instructions,
                                                       const Context &Ctx) const {
    using namespace LowLevelInstMatcherDSL;

    assert(Instructions.size() == 1 && "degenerate-branch rule requires one instruction");

    const Instruction &DI = Instructions.front();
    const MCInst &I = DI.Inst;

    if (!Ctx.MIA.isBranch(I) || Ctx.MIA.isCall(I) || Ctx.MIA.isIndirectBranch(I))
        return std::nullopt;

    uint64_t Target = 0;
    if (!Ctx.MIA.evaluateBranch(I, DI.Address, 4, Target))
        return std::nullopt;

    // A branch to the next instruction is reported by BranchToNextRule alone.
    if (Target == DI.Address + 4)
        return std::nullopt;

    const unsigned Op = I.getOpcode();

    // Two-register conditional branches: BEQ/BNE/BLT/BGE/BLTU/BGEU r, r.
    if (Op == LoongArch::BEQ || Op == LoongArch::BNE || Op == LoongArch::BLT ||
        Op == LoongArch::BGE || Op == LoongArch::BLTU || Op == LoongArch::BGEU) {
        Reg BranchReg;
        if (!matchInst(I, Op, BranchReg, BranchReg, Imm()))
            return std::nullopt;

        // Equal-or-greater comparisons on r,r are always true -> direct B.
        // Strictly-less or not-equal comparisons on r,r are always false -> delete.
        const bool AlwaysTrue =
            Op == LoongArch::BEQ || Op == LoongArch::BGE || Op == LoongArch::BGEU;
        if (!AlwaysTrue)
            return Match{};

        Rule::Match Result;
        Result.Replacement.emplace_back(
            MCInstBuilder(LoongArch::B).addImm(static_cast<int64_t>(Target - DI.Address)));
        return Result;
    }

    // Zero-register branches: BEQZ/BNEZ on $zero.
    if (Op == LoongArch::BEQZ || Op == LoongArch::BNEZ) {
        if (!matchInst(I, Op, Reg(LoongArch::R0), Imm()))
            return std::nullopt;

        if (Op == LoongArch::BNEZ)
            return Match{}; // BNEZ $zero: always false -> delete.

        Rule::Match Result; // BEQZ $zero: always true -> direct B.
        Result.Replacement.emplace_back(
            MCInstBuilder(LoongArch::B).addImm(static_cast<int64_t>(Target - DI.Address)));
        return Result;
    }

    return std::nullopt;
}

} // namespace loonglint
