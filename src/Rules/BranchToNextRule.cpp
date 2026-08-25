// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/Rules/BranchToNextRule.hpp"

#include <cassert>

using namespace llvm;

namespace loonglint {

StringRef BranchToNextRule::getID() const {
    return "control/branch-to-next";
}

StringRef BranchToNextRule::getDescription() const {
    return "delete branch to next instruction";
}

unsigned BranchToNextRule::getInstructionCount() const {
    return 1;
}

std::optional<Rule::Match> BranchToNextRule::match(ArrayRef<Instruction> Instructions,
                                                   const Context &Ctx) const {
    assert(Instructions.size() == 1 && "branch-to-next rule requires one instruction");

    const Instruction &DI = Instructions.front();
    if (!Ctx.MIA.isBranch(DI.Inst) || Ctx.MIA.isCall(DI.Inst) || Ctx.MIA.isIndirectBranch(DI.Inst))
        return std::nullopt;

    uint64_t Target = 0;
    if (!Ctx.MIA.evaluateBranch(DI.Inst, DI.Address, 4, Target) || Target != DI.Address + 4)
        return std::nullopt;

    return Match{};
}

} // namespace loonglint
