// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LOONGLINT_RISCV_RISCVSPEC_HPP
#define LOONGLINT_RISCV_RISCVSPEC_HPP

#include "loonglint/ArchSpec.hpp"

#include "llvm/ADT/StringRef.h"
#include "llvm/TargetParser/Triple.h"

#include <cstdint>
#include <memory>
#include <string>

namespace loonglint {

namespace RISCV {

class RISCVSpec final : public ArchSpec {
  public:
    // |Features| is the comma-separated MCSubtargetInfo feature string (e.g. "64bit,+m,+a,+c").
    RISCVSpec(unsigned XLen, std::string FeaturesString);

    // Mirrors ELFObjectFileBase::getRISCVFeatures().
    static std::unique_ptr<RISCVSpec> createFromArchString(llvm::StringRef Arch);

    llvm::StringRef getName() const override;
    unsigned getCellSize() const override;
    llvm::Triple getTriple() const override;
    uint16_t getELFMachine() const override;
    MCSubtarget getMCSubtarget() const override;
    llvm::SmallVector<std::unique_ptr<Rule>, 0> createRules() const override;

    bool isRV64() const {
        return XLen == 64;
    }

  private:
    unsigned XLen;
    std::string FeaturesString;
};

void queryArchSpec(std::unique_ptr<ArchSpec> &AS, const ArchQuery &AQ);

} // namespace RISCV

} // namespace loonglint

#endif
