// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LOONGLINT_RULES_SHIFTMASKRULE_HPP
#define LOONGLINT_RULES_SHIFTMASKRULE_HPP

#include "loonglint/LoongArchSpec.hpp"
#include "loonglint/Rule.hpp"

namespace loonglint {

class ShiftMaskRule final : public Rule {
  public:
    explicit ShiftMaskRule(const LoongArchSpec &LoongAS);

    llvm::StringRef getID() const override;
    llvm::StringRef getDescription() const override;
    unsigned getInstructionCount() const override;
    std::optional<Match> match(llvm::ArrayRef<Instruction> Instructions,
                               const Context &Ctx) const override;

  private:
    const LoongArchSpec &LoongAS;
};

} // namespace loonglint

#endif
