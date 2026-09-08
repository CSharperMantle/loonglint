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

} // namespace LoongArch

// Fragments the driver expands for this architecture while consuming
// loonglint/ArchConfig.def.
#define LOONGLINT_LoongArch_SPEC_CASES(AS)                                                         \
    if (!(AS) && Name == "loongarch64")                                                            \
        (AS) = std::make_unique<LoongArch::LoongArchSpec>(true);                                   \
    if (!(AS) && Name == "loongarch32")                                                            \
        (AS) = std::make_unique<LoongArch::LoongArchSpec>(false);

#define LOONGLINT_LoongArch_ELF_SPEC_CASES(AS, Machine, Is64)                                      \
    if (!(AS) && (Machine) == ELF::EM_LOONGARCH)                                                   \
        (AS) = std::make_unique<LoongArch::LoongArchSpec>(Is64);

} // namespace loonglint

#endif
