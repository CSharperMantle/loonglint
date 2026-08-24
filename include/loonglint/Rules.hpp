// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LOONGLINT_RULES_HPP
#define LOONGLINT_RULES_HPP

#include "loonglint/DisassemblerTarget.hpp"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrAnalysis.h"

#include <optional>

namespace loonglint {

struct RuleContext {
    Architecture Arch;
    const llvm::MCInstrAnalysis &MIA;
};

struct RuleMatch {
    llvm::SmallVector<llvm::MCInst, 0> Replacement;
};

using CheckFunction = std::optional<RuleMatch> (*)(llvm::ArrayRef<Instruction> Instructions,
                                                   const RuleContext &Ctx);

struct Rule {
    llvm::StringLiteral Id;
    llvm::StringLiteral Description;
    unsigned InstructionCount;
    CheckFunction Run;
};

unsigned maxInstCount(llvm::ArrayRef<Rule> Rules);
llvm::ArrayRef<Rule> rules();

} // namespace loonglint

#endif
