// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LOONGLINT_RULES_RISCV_BINVRULE_HPP
#define LOONGLINT_RULES_RISCV_BINVRULE_HPP

#include "loonglint/RISCV/RISCVSpec.hpp"
#include "loonglint/Rule.hpp"

namespace loonglint::RISCV {

class BinvRule final : public Rule {
  public:
    explicit BinvRule(const RISCVSpec &RISCVAS);

    llvm::StringRef getID() const override;
    llvm::StringRef getDescription() const override;
    unsigned getInstructionCount() const override;
    bool shouldRun(const Context &Ctx) const override;
    std::optional<Match> match(llvm::ArrayRef<Instruction> Instructions,
                               const Context &Ctx) const override;

  private:
    const RISCVSpec &RISCVAS;
};

} // namespace loonglint::RISCV

#endif
