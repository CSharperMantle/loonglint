// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LOONGLINT_RULEMANAGER_HPP
#define LOONGLINT_RULEMANAGER_HPP

#include "loonglint/Rule.hpp"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/STLFunctionalExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/iterator.h"

#include <cstdint>
#include <memory>

namespace loonglint {

struct Finding {
    const Rule &MatchedRule;
    llvm::ArrayRef<Instruction> Instructions;
    const Rule::Match &Match;
};

using FindingHandler = llvm::function_ref<void(const Finding &)>;

class RuleManager final {
  public:
    RuleManager();

    auto rules() const {
        return llvm::make_pointee_range(Rules);
    }

    unsigned maxInstructionCount() const;

    uint64_t runWindow(llvm::ArrayRef<Instruction> Window, const Rule::Context &Ctx,
                       FindingHandler HandleFinding) const;

  private:
    void registerRule(std::unique_ptr<Rule> NewRule);

    llvm::SmallVector<std::unique_ptr<Rule>, 0> Rules;
};

} // namespace loonglint

#endif
