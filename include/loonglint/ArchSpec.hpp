// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LOONGLINT_ARCHSPEC_HPP
#define LOONGLINT_ARCHSPEC_HPP

#include "loonglint/Rule.hpp"

#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/TargetParser/SubtargetFeature.h"
#include "llvm/TargetParser/Triple.h"

#include <cstdint>
#include <memory>
#include <variant>

namespace loonglint {

struct MCSubtarget {
    llvm::StringRef CPU;      // "" for LoongArch today
    llvm::StringRef Features; // "" for LoongArch today; "+m,+a,+c,..." for RISC-V profiles
};

// Information needed to make an ArchSpec. This is consumed by
// loonglint::<ARCH>::queryArchSpec().
struct ArchQuery {
    struct Raw {
        // --arch value.
        llvm::StringRef Name;
    };
    struct ELF {
        uint16_t EMachine = 0;
        bool Is64 = false;
        llvm::SubtargetFeatures Features;
    };

    std::variant<Raw, ELF> Data;
};

class ArchSpec {
  public:
    virtual ~ArchSpec() = default;

    virtual llvm::StringRef getName() const = 0;
    virtual unsigned getCellSize() const = 0;
    virtual llvm::Triple getTriple() const = 0;
    virtual uint16_t getELFMachine() const = 0;
    virtual MCSubtarget getMCSubtarget() const = 0;
    virtual llvm::SmallVector<std::unique_ptr<Rule>, 0> createRules() const = 0;
};

} // namespace loonglint

#endif
