// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LOONGLINT_RULE_HPP
#define LOONGLINT_RULE_HPP

#include "loonglint/DisassemblerTarget.hpp"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrAnalysis.h"

#include <optional>

namespace loonglint {

class Rule {
  public:
    class Context {
      public:
        Architecture Arch;
        const llvm::MCInstrAnalysis &MIA;

        explicit Context(const DisassemblerTarget &DT) : Arch(DT.Arch), MIA(*DT.MIA) {}
    };

    struct Match {
        llvm::SmallVector<llvm::MCInst, 0> Replacement;
    };

    virtual ~Rule() = default;

    virtual llvm::StringRef getID() const = 0;
    virtual llvm::StringRef getDescription() const = 0;
    virtual unsigned getInstructionCount() const = 0;
    virtual bool shouldRun(const Context &) const {
        return true;
    }

    virtual std::optional<Match> match(llvm::ArrayRef<Instruction> Instructions,
                                       const Context &Ctx) const = 0;
};

} // namespace loonglint

#endif
