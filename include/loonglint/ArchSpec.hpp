// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LOONGLINT_ARCHSPEC_HPP
#define LOONGLINT_ARCHSPEC_HPP

#include "loonglint/Rule.hpp"

#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/TargetParser/Triple.h"

#include <cstdint>
#include <memory>

namespace loonglint {

struct MCSubtarget {
    llvm::StringRef CPU;      // "" for LoongArch today
    llvm::StringRef Features; // "" for LoongArch today; "+m,+a,+c,..." for RISC-V profiles
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
