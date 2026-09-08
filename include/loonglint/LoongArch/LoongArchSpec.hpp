// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LOONGLINT_LOONGARCH_LOONGARCHSPEC_HPP
#define LOONGLINT_LOONGARCH_LOONGARCHSPEC_HPP

#include "loonglint/ArchSpec.hpp"

#include "llvm/ADT/StringRef.h"
#include "llvm/TargetParser/Triple.h"

#include <cstdint>
#include <memory>

namespace loonglint {

namespace LoongArch {

class LoongArchSpec final : public ArchSpec {
  public:
    explicit LoongArchSpec(bool Is64);

    llvm::StringRef getName() const override;
    unsigned getCellSize() const override;
    llvm::Triple getTriple() const override;
    uint16_t getELFMachine() const override;
    MCSubtarget getMCSubtarget() const override;
    llvm::SmallVector<std::unique_ptr<Rule>, 0> createRules() const override;

    bool is64() const {
        return Is64;
    }

  private:
    bool Is64;
};

void queryArchSpec(std::unique_ptr<ArchSpec> &AS, const ArchQuery &AQ);

} // namespace LoongArch

} // namespace loonglint

#endif
