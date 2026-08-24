// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/Rules.hpp"

#include "loonglint/MCInstMatcher.hpp"

#include "MCTargetDesc/LoongArchMCTargetDesc.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/MC/MCRegister.h"
#include "llvm/Support/Debug.h"

#include <algorithm>
#include <cassert>
#include <optional>

using namespace llvm;

#ifdef DEBUG_TYPE
#undef DEBUG_TYPE
#endif
#define DEBUG_TYPE "rules"

namespace loonglint {

static std::optional<RuleMatch> checkSelfMove(ArrayRef<Instruction> Instructions,
                                              const RuleContext &) {
    using namespace LowLevelInstMatcherDSL;

    assert(Instructions.size() == 1 && "self-move rule requires one instruction");

    Reg Rd, Rj, Rk;
    if (!matchInst(Instructions.front().Inst, LoongArch::OR, Rd, Rj, Rk))
        return std::nullopt;

    const bool RightZeroMove = Rd.get() == Rj.get() && Rk.get() == LoongArch::R0;
    const bool LeftZeroMove = Rd.get() == Rk.get() && Rj.get() == LoongArch::R0;
    if (!RightZeroMove && !LeftZeroMove)
        return std::nullopt;

    return RuleMatch{};
}

static std::optional<RuleMatch> checkBranchToNext(ArrayRef<Instruction> Instructions,
                                                  const RuleContext &Ctx) {
    assert(Instructions.size() == 1 && "branch-to-next rule requires one instruction");

    const Instruction &DI = Instructions.front();
    if (!Ctx.MIA.isBranch(DI.Inst) || Ctx.MIA.isCall(DI.Inst) || Ctx.MIA.isIndirectBranch(DI.Inst))
        return std::nullopt;

    uint64_t Target = 0;
    if (!Ctx.MIA.evaluateBranch(DI.Inst, DI.Address, 4, Target) || Target != DI.Address + 4)
        return std::nullopt;

    return RuleMatch{};
}

static constexpr Rule RuleRegistry[] = {
    {"integer/self-move", "delete redundant self-move", 1, checkSelfMove},
    {"control/branch-to-next", "delete branch to next instruction", 1, checkBranchToNext},
};

unsigned maxInstCount(ArrayRef<Rule> Rules) {
    unsigned Result = 0;
    for (const auto &R : Rules)
        Result = std::max(Result, R.InstructionCount);
    return Result;
}

ArrayRef<Rule> rules() {
    LLVM_DEBUG({
        assert(maxInstCount(RuleRegistry) != 0 && "rule registry contains only zero-sized rules");
        for (const auto &R : RuleRegistry) {
            assert(R.InstructionCount != 0 && "rule registry contains a zero-sized rule");
        }
    });

    return RuleRegistry;
}

} // namespace loonglint
