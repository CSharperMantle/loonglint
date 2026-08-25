// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/Rules/NopRule.hpp"

#include "loonglint/MCInstMatcher.hpp"

#include "MCTargetDesc/LoongArchMCTargetDesc.h"

#include <cassert>

using namespace llvm;

namespace loonglint {

StringRef NopRule::getID() const {
    return "integer/nop";
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
    case LoongArch::OR: {
        // OR rd, rd, $zero | OR rd, $zero, rd | OR rd, rd, rd
        Reg RdReg, RjReg, RkReg;
        if (!matchInst(I, Opcode, RdReg, RjReg, RkReg))
            return std::nullopt;
        const MCRegister Rd = RdReg.get();
        const MCRegister Rj = RjReg.get();
        const MCRegister Rk = RkReg.get();

        const bool JIsRd = Rj == Rd;
        const bool KIsRd = Rk == Rd;
        if ((JIsRd && (Rk == LoongArch::R0 || KIsRd)) || (KIsRd && (Rj == LoongArch::R0 || JIsRd)))
            return Match{};
        return std::nullopt;
    }
    case LoongArch::AND: {
        // AND rd, rd, rd.
        Reg RdReg;
        if (matchInst(I, Opcode, RdReg, RdReg, RdReg))
            return Match{};
        return std::nullopt;
    }
    case LoongArch::ANDN: {
        // ANDN rd, rd, $zero = rd & ~0 = rd; the reverse operand order yields 0.
        Reg RdReg;
        if (matchInst(I, Opcode, RdReg, RdReg, Reg(LoongArch::R0)))
            return Match{};
        return std::nullopt;
    }
    case LoongArch::XOR: {
        // XOR rd, rd, $zero | XOR rd, $zero, rd
        // (Excluding XOR rd, rd, rd, which would zero)
        Reg RdReg, RjReg, RkReg;
        if (!matchInst(I, Opcode, RdReg, RjReg, RkReg))
            return std::nullopt;

        const MCRegister Rd = RdReg.get();
        const MCRegister Rj = RjReg.get();
        const MCRegister Rk = RkReg.get();
        if ((Rj == Rd && Rk == LoongArch::R0) || (Rk == Rd && Rj == LoongArch::R0))
            return Match{};
        return std::nullopt;
    }
    case LoongArch::ORI: {
        // ORI rd, rd, 0
        Reg RdReg;
        if (matchInst(I, Opcode, RdReg, RdReg, Imm(0)))
            return Match{};
        return std::nullopt;
    }
    case LoongArch::XORI: {
        // XORI rd, rd, 0
        Reg RdReg;
        if (matchInst(I, Opcode, RdReg, RdReg, Imm(0)))
            return Match{};
        return std::nullopt;
    }
    default:
        return std::nullopt;
    }

    return std::nullopt;
}

} // namespace loonglint
