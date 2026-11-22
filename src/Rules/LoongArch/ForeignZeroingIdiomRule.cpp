// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/Rules/LoongArch/ForeignZeroingIdiomRule.hpp"

#include "loonglint/LoongArch/LoongArchInstr.hpp"
#include "loonglint/MCInstMatcher.hpp"

#include "MCTargetDesc/LoongArchMCTargetDesc.h"

#include "llvm/MC/MCInstBuilder.h"

#include <cassert>

using namespace llvm;

namespace loonglint::LoongArch {

namespace LoongArch = ::llvm::LoongArch;

StringRef ForeignZeroingIdiomRule::getID() const {
    return "loongarch:integer/foreign-zeroing-idiom";
}

StringRef ForeignZeroingIdiomRule::getDescription() const {
    return "replace foreign zeroing idiom with 'move rd, $zero'";
}

unsigned ForeignZeroingIdiomRule::getInstructionCount() const {
    return 1;
}

std::optional<Rule::Match> ForeignZeroingIdiomRule::match(ArrayRef<Instruction> Instructions,
                                                          const Context &) const {
    using namespace LowLevelInstMatcherDSL;

    assert(Instructions.size() == 1 && "integer/foreign-zeroing-idiom requires one instruction");

    const MCInst &I = Instructions.front().Inst;

    // `li.[wd] rd, 0` -> `move rd, zero` is already canonical per GCC.
    if (matchInst(I, LoongArch::OR, Reg(), Reg(LoongArch::R0), Reg(LoongArch::R0)))
        return std::nullopt;
    // `li.[wd] rd, 0` -> `ori rd, zero, 0` is already canonical per LLVM.
    if (matchInst(I, LoongArch::ORI, Reg(), Reg(LoongArch::R0), Imm(0)))
        return std::nullopt;

    const std::optional<MCRegister> ZeroRd = matchZeroWrite(I);
    if (!ZeroRd || *ZeroRd == LoongArch::R0)
        return std::nullopt;

    Rule::Match Result;
    Result.Replacement.emplace_back(
        MCInstBuilder(LoongArch::OR).addReg(*ZeroRd).addReg(LoongArch::R0).addReg(LoongArch::R0));
    return Result;
}

} // namespace loonglint::LoongArch
