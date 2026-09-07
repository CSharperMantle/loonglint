// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LOONGLINT_RULES_LOONGARCH_BYTEREVERSERULE_HPP
#define LOONGLINT_RULES_LOONGARCH_BYTEREVERSERULE_HPP

#include "loonglint/LoongArchSpec.hpp"
#include "loonglint/Rule.hpp"

namespace loonglint::LoongArch {

class ByteReverseRule final : public Rule {
  public:
    explicit ByteReverseRule(const LoongArchSpec &LoongAS);

    llvm::StringRef getID() const override;
    llvm::StringRef getDescription() const override;
    unsigned getInstructionCount() const override;
    bool shouldRun(const Context &Ctx) const override;
    std::optional<Match> match(llvm::ArrayRef<Instruction> Instructions,
                               const Context &Ctx) const override;

  private:
    const LoongArchSpec &LoongAS;
};

} // namespace loonglint::LoongArch

#endif
