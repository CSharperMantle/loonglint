// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/RuleManager.hpp"

#include "loonglint/ArchSpec.hpp"

#include "llvm/Support/Debug.h"

#include <algorithm>
#include <cassert>
#include <utility>

#ifdef DEBUG_TYPE
#undef DEBUG_TYPE
#endif
#define DEBUG_TYPE "rule-manager"

using namespace llvm;

namespace loonglint {

RuleManager::RuleManager(const DisassemblerTarget &DT, const RuleFilter &Filter)
    : DT(DT), Filter(Filter) {
    for (std::unique_ptr<Rule> &R : DT.AS.createRules())
        registerRule(std::move(R));
}

unsigned RuleManager::getMaxInstCount() const {
    unsigned Result = 0;
    for (const auto &R : getRules())
        Result = std::max(Result, R.getInstructionCount());
    return Result;
}

uint64_t RuleManager::runWindow(ArrayRef<Instruction> Window, FindingHandler HandleFinding) const {
    const Rule::Context Ctx(DT);
    uint64_t FindingCount = 0;
    for (const auto &R : getRules()) {
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
        const StringRef ID = NewRule->getID();
        assert(!ID.empty() && "cannot register a rule without an ID");
        assert(NewRule->getInstructionCount() != 0 && "rule has zero instruction count");

        for (const auto &ExistingRule : getRules())
            assert(ExistingRule.getID() != ID && "duplicate rule ID");
    });

    if (Filter.excludes(NewRule->getID()))
        return;

    Rules.emplace_back(std::move(NewRule));
}

} // namespace loonglint
