// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LOONGLINT_RULES_NOPLA32RULE_HPP
#define LOONGLINT_RULES_NOPLA32RULE_HPP

#include "loonglint/Rule.hpp"

namespace loonglint {

class NopLA32Rule final : public Rule {
  public:
    llvm::StringRef getID() const override;
    llvm::StringRef getDescription() const override;
    unsigned getInstructionCount() const override;
    bool shouldRun(const Context &Ctx) const override;
    std::optional<Match> match(llvm::ArrayRef<Instruction> Instructions,
                               const Context &Ctx) const override;
};

} // namespace loonglint

#endif
