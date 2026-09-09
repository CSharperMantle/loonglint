// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LOONGLINT_RULES_RISCV_ADDRESSLOADRULE_HPP
#define LOONGLINT_RULES_RISCV_ADDRESSLOADRULE_HPP

#include "loonglint/RISCV/RISCVSpec.hpp"
#include "loonglint/Rule.hpp"

namespace loonglint::RISCV {

class AddressLoadRule final : public Rule {
  public:
    explicit AddressLoadRule(const RISCVSpec &RISCVAS);

    llvm::StringRef getID() const override;
    llvm::StringRef getDescription() const override;
    unsigned getInstructionCount() const override;
    std::optional<Match> match(llvm::ArrayRef<Instruction> Instructions,
                               const Context &Ctx) const override;

  private:
    const RISCVSpec &RISCVAS;
};

} // namespace loonglint::RISCV

#endif
