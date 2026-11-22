// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/Rules/RISCV/ForeignZeroingIdiomRule.hpp"

#include "loonglint/MCInstMatcher.hpp"
#include "loonglint/RISCV/RISCVInstr.hpp"

#include "MCTargetDesc/RISCVMCTargetDesc.h"

#include "llvm/MC/MCInstBuilder.h"

#include <cassert>

using namespace llvm;

namespace loonglint::RISCV {

namespace RISCV = ::llvm::RISCV;

StringRef ForeignZeroingIdiomRule::getID() const {
    return "riscv:integer/foreign-zeroing-idiom";
}

StringRef ForeignZeroingIdiomRule::getDescription() const {
    return "replace foreign zeroing idiom with 'mv rd, zero'";
}

unsigned ForeignZeroingIdiomRule::getInstructionCount() const {
    return 1;
}

std::optional<Rule::Match> ForeignZeroingIdiomRule::match(ArrayRef<Instruction> Instructions,
                                                          const Context &) const {
    using namespace LowLevelInstMatcherDSL;

    assert(Instructions.size() == 1 && "integer/foreign-zeroing-idiom requires one instruction");

    const MCInst &I = Instructions.front().Inst;
    const unsigned Opcode = I.getOpcode();

    if (classifyHint(I))
        return std::nullopt;
    // c.li rd, 0 is already canonical.
    if (matchInst(I, RISCV::C_LI, Reg(), Imm(0)))
        return std::nullopt;
    // mv rd, zero is already canonical.
    if (matchInst(I, RISCV::ADDI, Reg(), Reg(RISCV::X0), Imm(0)))
        return std::nullopt;

    const std::optional<MCRegister> ZeroRd = matchZeroWrite(I);
    if (!ZeroRd)
        return std::nullopt;

    Rule::Match Result;
    if (Opcode == RISCV::C_XOR)
        Result.Replacement.emplace_back(MCInstBuilder(RISCV::C_LI).addReg(*ZeroRd).addImm(0));
    else
        Result.Replacement.emplace_back(
            MCInstBuilder(RISCV::ADDI).addReg(*ZeroRd).addReg(RISCV::X0).addImm(0));
    return Result;
}

} // namespace loonglint::RISCV
