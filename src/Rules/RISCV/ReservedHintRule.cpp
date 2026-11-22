// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/Rules/RISCV/ReservedHintRule.hpp"

#include "loonglint/RISCV/RISCVInstr.hpp"

#include <cassert>

using namespace llvm;

namespace loonglint::RISCV {

StringRef ReservedHintRule::getID() const {
    return "riscv:integer/reserved-hint";
}

StringRef ReservedHintRule::getDescription() const {
    return "delete HINT instructions reserved for future standard use";
}

unsigned ReservedHintRule::getInstructionCount() const {
    return 1;
}

std::optional<Rule::Match> ReservedHintRule::match(ArrayRef<Instruction> Instructions,
                                                   const Context &) const {
    assert(Instructions.size() == 1 && "integer/reserved-hint requires one instruction");

    if (classifyHint(Instructions.front().Inst) == HintKind::FutureStandard)
        return Match{};
    return std::nullopt;
}

} // namespace loonglint::RISCV
