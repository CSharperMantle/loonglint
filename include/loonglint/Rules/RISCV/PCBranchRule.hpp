// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LOONGLINT_RULES_RISCV_PCBRANCHRULE_HPP
#define LOONGLINT_RULES_RISCV_PCBRANCHRULE_HPP

#include "loonglint/Rule.hpp"

namespace loonglint::RISCV {

class PCBranchRule final : public Rule {
  public:
    llvm::StringRef getID() const override;
    llvm::StringRef getDescription() const override;
    unsigned getInstructionCount() const override;
    std::optional<Match> match(llvm::ArrayRef<Instruction> Instructions,
                               const Context &Ctx) const override;
};

} // namespace loonglint::RISCV

#endif
