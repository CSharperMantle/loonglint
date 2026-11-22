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

    Reg RdReg, RsReg;
    switch (Opcode) {
    case RISCV::XOR:
    case RISCV::SUB:
    case RISCV::SUBW:
    case RISCV::SLT:
    case RISCV::SLTU:
        if (!matchInst(I, Opcode, RdReg, RsReg, RsReg))
            return std::nullopt;
        break;
    case RISCV::AND: {
        Reg Rs1Reg, Rs2Reg;
        if (!matchInst(I, Opcode, RdReg, Rs1Reg, Rs2Reg) ||
            (Rs1Reg.get() != RISCV::X0 && Rs2Reg.get() != RISCV::X0))
            return std::nullopt;
        break;
    }
    case RISCV::ANDI: {
        Imm TheImm;
        if (!matchInst(I, Opcode, RdReg, RsReg, TheImm) ||
            (RsReg.get() != RISCV::X0 && TheImm.get() != 0))
            return std::nullopt;
        break;
    }
    case RISCV::SLTIU:
        if (!matchInst(I, Opcode, RdReg, Reg(), Imm(0)))
            return std::nullopt;
        break;
    case RISCV::SLLI:
    case RISCV::SRLI:
    case RISCV::SRAI:
        if (!matchInst(I, Opcode, RdReg, Reg(RISCV::X0), Imm()))
            return std::nullopt;
        break;
    case RISCV::SLL:
    case RISCV::SRL:
    case RISCV::SRA:
        if (!matchInst(I, Opcode, RdReg, Reg(RISCV::X0), Reg()))
            return std::nullopt;
        break;
    case RISCV::LUI:
        if (!matchInst(I, Opcode, RdReg, Imm(0)))
            return std::nullopt;
        break;
    case RISCV::ADD:
    case RISCV::OR:
        if (!matchInst(I, Opcode, RdReg, Reg(RISCV::X0), Reg(RISCV::X0)))
            return std::nullopt;
        break;
    case RISCV::SLLIW:
    case RISCV::SRLIW:
    case RISCV::SRAIW:
        if (!matchInst(I, Opcode, RdReg, Reg(RISCV::X0), Imm()))
            return std::nullopt;
        break;
    case RISCV::SLLW:
    case RISCV::SRLW:
    case RISCV::SRAW:
        if (!matchInst(I, Opcode, RdReg, Reg(RISCV::X0), Reg()))
            return std::nullopt;
        break;
    case RISCV::ADDW:
        if (!matchInst(I, Opcode, RdReg, Reg(RISCV::X0), Reg(RISCV::X0)))
            return std::nullopt;
        break;
    case RISCV::C_XOR: {
        if (!matchInst(I, Opcode, RdReg, RdReg, RdReg))
            return std::nullopt;
        Rule::Match Result;
        Result.Replacement.emplace_back(MCInstBuilder(RISCV::C_LI).addReg(RdReg.get()).addImm(0));
        return Result;
    }
    default:
        return std::nullopt;
    }

    Rule::Match Result;
    Result.Replacement.emplace_back(
        MCInstBuilder(RISCV::ADDI).addReg(RdReg.get()).addReg(RISCV::X0).addImm(0));
    return Result;
}

} // namespace loonglint::RISCV
