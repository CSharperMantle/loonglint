// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/Rules/SelfMoveRule.hpp"

#include "loonglint/MCInstMatcher.hpp"

#include "MCTargetDesc/LoongArchMCTargetDesc.h"

#include <cassert>

using namespace llvm;

namespace loonglint {

StringRef SelfMoveRule::getID() const {
    return "integer/self-move";
}

StringRef SelfMoveRule::getDescription() const {
    return "delete redundant self-move";
}

unsigned SelfMoveRule::getInstructionCount() const {
    return 1;
}

std::optional<Rule::Match> SelfMoveRule::match(ArrayRef<Instruction> Instructions,
                                               const Context &) const {
    using namespace LowLevelInstMatcherDSL;

    assert(Instructions.size() == 1 && "self-move rule requires one instruction");

    Reg Rd, Rj, Rk;
    if (!matchInst(Instructions.front().Inst, LoongArch::OR, Rd, Rj, Rk))
        return std::nullopt;

    const bool RightZeroMove = Rd.get() == Rj.get() && Rk.get() == LoongArch::R0;
    const bool LeftZeroMove = Rd.get() == Rk.get() && Rj.get() == LoongArch::R0;
    if (!RightZeroMove && !LeftZeroMove)
        return std::nullopt;

    return Match{};
}

} // namespace loonglint
