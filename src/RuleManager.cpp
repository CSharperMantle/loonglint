// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/RuleManager.hpp"

#include "loonglint/Rules/AddiPairRule.hpp"
#include "loonglint/Rules/AddressLoadRule.hpp"
#include "loonglint/Rules/AndNotRule.hpp"
#include "loonglint/Rules/BitCountRule.hpp"
#include "loonglint/Rules/BitExtractRule.hpp"
#include "loonglint/Rules/BitReverseRule.hpp"
#include "loonglint/Rules/BranchToNextRule.hpp"
#include "loonglint/Rules/ByteReverseRule.hpp"
#include "loonglint/Rules/DegenerateBranchRule.hpp"
#include "loonglint/Rules/IndexedLoadRule.hpp"
#include "loonglint/Rules/LoadExtendRule.hpp"
#include "loonglint/Rules/LoadZeroExtendRule.hpp"
#include "loonglint/Rules/MulhSextRule.hpp"
#include "loonglint/Rules/NopLA32Rule.hpp"
#include "loonglint/Rules/NopLA64Rule.hpp"
#include "loonglint/Rules/NopRule.hpp"
#include "loonglint/Rules/NotOrRule.hpp"
#include "loonglint/Rules/OrNotRule.hpp"
#include "loonglint/Rules/RotateCombineRule.hpp"
#include "loonglint/Rules/ShiftAddAlslDRule.hpp"
#include "loonglint/Rules/ShiftChainRule.hpp"
#include "loonglint/Rules/ShiftDoubleRule.hpp"
#include "loonglint/Rules/ShiftMaskRule.hpp"
#include "loonglint/Rules/ZeroExtendRule.hpp"

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
    registerRule(std::make_unique<NopRule>());
    registerRule(std::make_unique<NopLA32Rule>());
    registerRule(std::make_unique<NopLA64Rule>());
    registerRule(std::make_unique<BitExtractRule>());
    registerRule(std::make_unique<ZeroExtendRule>());
    registerRule(std::make_unique<BitReverseRule>());
    registerRule(std::make_unique<ByteReverseRule>());
    registerRule(std::make_unique<MulhSextRule>());
    registerRule(std::make_unique<ShiftChainRule>());
    registerRule(std::make_unique<ShiftDoubleRule>());
    registerRule(std::make_unique<RotateCombineRule>());
    registerRule(std::make_unique<ShiftMaskRule>());
    registerRule(std::make_unique<AndNotRule>());
    registerRule(std::make_unique<OrNotRule>());
    registerRule(std::make_unique<NotOrRule>());
    registerRule(std::make_unique<BitCountRule>());
    registerRule(std::make_unique<BranchToNextRule>());
    registerRule(std::make_unique<DegenerateBranchRule>());
    registerRule(std::make_unique<ShiftAddAlslDRule>());
    registerRule(std::make_unique<AddiPairRule>());
    registerRule(std::make_unique<AddressLoadRule>());
    registerRule(std::make_unique<LoadExtendRule>());
    registerRule(std::make_unique<IndexedLoadRule>());
    registerRule(std::make_unique<LoadZeroExtendRule>());
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
        const StringRef ID = NewRule->getID();
        assert(!ID.empty() && "cannot register a rule without an ID");
        assert(NewRule->getInstructionCount() != 0 && "rule has zero instruction count");

        for (const auto &ExistingRule : rules())
            assert(ExistingRule.getID() != ID && "duplicate rule ID");
    });

    Rules.emplace_back(std::move(NewRule));
}

} // namespace loonglint
