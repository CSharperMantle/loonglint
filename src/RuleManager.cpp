// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/RuleManager.hpp"

#include "loonglint/Rules/BranchToNextRule.hpp"
#include "loonglint/Rules/SelfMoveRule.hpp"
#include "loonglint/Rules/ShiftAddAlslDRule.hpp"

#include "llvm/Support/Debug.h"

#include <algorithm>
#include <cassert>
#include <memory>

#ifdef DEBUG_TYPE
#undef DEBUG_TYPE
#endif
#define DEBUG_TYPE "rule-manager"

using namespace llvm;

namespace loonglint {

RuleManager::RuleManager(const DisassemblerTarget &DT) : DT(DT) {
    registerRule(std::make_unique<SelfMoveRule>());
    registerRule(std::make_unique<BranchToNextRule>());
    registerRule(std::make_unique<ShiftAddAlslDRule>());
}

unsigned RuleManager::maxInstructionCount() const {
    unsigned Result = 0;
    for (const auto &R : rules())
        Result = std::max(Result, R.getInstructionCount());
    return Result;
}

uint64_t RuleManager::runWindow(ArrayRef<Instruction> Window, FindingHandler HandleFinding) const {
    const Rule::Context Ctx(DT);
    uint64_t FindingCount = 0;
    for (const auto &R : rules()) {
        const unsigned InstructionCount = R.getInstructionCount();
        if (InstructionCount > Window.size() || !R.shouldRun(Ctx))
            continue;

        const ArrayRef<Instruction> Instructions(Window.data(), InstructionCount);
        if (std::optional<Rule::Match> Match = R.match(Instructions, Ctx)) {
            HandleFinding({R, Instructions, *Match});
            ++FindingCount;
        }
    }
    return FindingCount;
}

void RuleManager::registerRule(std::unique_ptr<Rule> NewRule) {
    LLVM_DEBUG({
        assert(NewRule && "cannot register a null rule");
        const StringRef Id = NewRule->getID();
        assert(!Id.empty() && "cannot register a rule without an ID");
        assert(NewRule->getInstructionCount() != 0 && "rule has zero instruction count");

        for (const auto &ExistingRule : rules())
            assert(ExistingRule.getID() != Id && "duplicate rule ID");
    });

    Rules.emplace_back(std::move(NewRule));
}

} // namespace loonglint
