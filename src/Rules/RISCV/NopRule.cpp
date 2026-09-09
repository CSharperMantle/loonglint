// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/Rules/RISCV/NopRule.hpp"

#include "loonglint/MCInstMatcher.hpp"

#include "MCTargetDesc/RISCVMCTargetDesc.h"

#include <cassert>

using namespace llvm;

namespace loonglint::RISCV {

namespace RISCV = ::llvm::RISCV;

StringRef NopRule::getID() const {
    return "riscv:integer/nop";
}

StringRef NopRule::getDescription() const {
    return "delete or replace non-canonical NOP instruction";
}

unsigned NopRule::getInstructionCount() const {
    return 1;
}

std::optional<Rule::Match> NopRule::match(ArrayRef<Instruction> Instructions,
                                          const Context &) const {
    using namespace LowLevelInstMatcherDSL;

    assert(Instructions.size() == 1 && "nop rule requires one instruction");

    const MCInst &I = Instructions.front().Inst;
    const unsigned Opcode = I.getOpcode();

    switch (Opcode) {
    case RISCV::ADDI:
    case RISCV::ORI:
    case RISCV::XORI:
    case RISCV::SLLI:
    case RISCV::SRLI:
    case RISCV::SRAI: {
        // addi/ori/xori/slli/srli/srai rd, rd, 0.
        Reg RdReg;
        if (matchInst(I, Opcode, RdReg, RdReg, Imm(0)) && RdReg.get() != RISCV::X0)
            return Match{};
        return std::nullopt;
    }
    case RISCV::ANDI: {
        // andi rd, rd, -1: and with the all-ones sign-extended immediate.
        Reg RdReg;
        if (matchInst(I, Opcode, RdReg, RdReg, Imm(-1)) && RdReg.get() != RISCV::X0)
            return Match{};
        return std::nullopt;
    }
    case RISCV::ADD:
    case RISCV::XOR: {
        // add rd, rd, x0 | add rd, x0, rd; xor likewise.
        // (add rd, rd, rd doubles and xor rd, rd, rd zeroes, so the NOP
        // holds only when one operand register is x0.)
        Reg RdReg, Rs1Reg, Rs2Reg;
        if (!matchInst(I, Opcode, RdReg, Rs1Reg, Rs2Reg))
            return std::nullopt;

        const MCRegister Rd = RdReg.get();
        const MCRegister Rs1 = Rs1Reg.get();
        const MCRegister Rs2 = Rs2Reg.get();
        if (Rd != RISCV::X0 && ((Rs1 == Rd && Rs2 == RISCV::X0) || (Rs2 == Rd && Rs1 == RISCV::X0)))
            return Match{};
        return std::nullopt;
    }
    case RISCV::OR: {
        // or rd, rd, x0 | or rd, x0, rd | or rd, rd, rd.
        Reg RdReg, Rs1Reg, Rs2Reg;
        if (!matchInst(I, Opcode, RdReg, Rs1Reg, Rs2Reg))
            return std::nullopt;

        const MCRegister Rd = RdReg.get();
        const MCRegister Rs1 = Rs1Reg.get();
        const MCRegister Rs2 = Rs2Reg.get();
        if (Rd != RISCV::X0 && ((Rs1 == Rd && Rs2 == RISCV::X0) ||
                                (Rs2 == Rd && Rs1 == RISCV::X0) || (Rs1 == Rd && Rs2 == Rd)))
            return Match{};
        return std::nullopt;
    }
    case RISCV::AND: {
        // and rd, rd, rd. (and rd, rd, x0 zeroes.)
        Reg RdReg;
        if (matchInst(I, Opcode, RdReg, RdReg, RdReg) && RdReg.get() != RISCV::X0)
            return Match{};
        return std::nullopt;
    }
    case RISCV::SUB:
    case RISCV::SLL:
    case RISCV::SRL:
    case RISCV::SRA: {
        // sub/sll/srl/sra rd, rd, x0: subtracting zero shifts by zero.
        // (sub rd, x0, rd negates and the s* rd, x0, rs2 forms zero.)
        Reg RdReg;
        if (matchInst(I, Opcode, RdReg, RdReg, Reg(RISCV::X0)) && RdReg.get() != RISCV::X0)
            return Match{};
        return std::nullopt;
    }
    case RISCV::C_ADDI:
    case RISCV::C_SLLI: {
        // c.addi/c.slli rd, 0 (rd=x0 encodes c.nop or a HINT).
        Reg RdReg;
        if (matchInst(I, Opcode, RdReg, RdReg, Imm(0)) && RdReg.get() != RISCV::X0)
            return Match{};
        return std::nullopt;
    }
    case RISCV::C_SRLI:
    case RISCV::C_SRAI: {
        // c.srli/c.srai rd', 0. rd' spans x8--x15, never x0.
        Reg RdReg;
        if (matchInst(I, Opcode, RdReg, RdReg, Imm(0)))
            return Match{};
        return std::nullopt;
    }
    case RISCV::C_ANDI: {
        // c.andi rd', -1. rd' spans x8--x15, never x0.
        Reg RdReg;
        if (matchInst(I, Opcode, RdReg, RdReg, Imm(-1)))
            return Match{};
        return std::nullopt;
    }
    case RISCV::C_MV: {
        // c.mv rd, rd. The decoder guarantees rs2 != x0, so rd == rs2 already
        // implies rd != x0 (c.mv with rd=x0 is a HINT, not a NOP).
        Reg RdReg, Rs2Reg;
        if (matchInst(I, Opcode, RdReg, Rs2Reg) && RdReg.get() == Rs2Reg.get())
            return Match{};
        return std::nullopt;
    }
    case RISCV::C_AND:
    case RISCV::C_OR: {
        // c.and/c.or rd', rd'. rd' spans x8--x15, never x0.
        // (c.xor rd', rd' zeroes; c.add rd, rd doubles, and with rs2=x0 the
        // encoding is c.jalr/c.ebreak instead.)
        Reg RdReg, Rs2Reg;
        if (matchInst(I, Opcode, RdReg, RdReg, Rs2Reg) && RdReg.get() == Rs2Reg.get())
            return Match{};
        return std::nullopt;
    }
    default:
        return std::nullopt;
    }

    return std::nullopt;
}

} // namespace loonglint::RISCV
