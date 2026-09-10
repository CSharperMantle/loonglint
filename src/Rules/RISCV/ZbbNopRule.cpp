// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/Rules/RISCV/ZbbNopRule.hpp"

#include "loonglint/MCInstMatcher.hpp"

#include "MCTargetDesc/RISCVMCTargetDesc.h"

#include <cassert>

using namespace llvm;

namespace loonglint::RISCV {

namespace RISCV = ::llvm::RISCV;

ZbbNopRule::ZbbNopRule(const RISCVSpec &RISCVAS) : RISCVAS(RISCVAS) {}

StringRef ZbbNopRule::getID() const {
    return "riscv:integer/zbb-nop";
}

StringRef ZbbNopRule::getDescription() const {
    return "delete or replace non-canonical Zbb nop instruction";
}

unsigned ZbbNopRule::getInstructionCount() const {
    return 1;
}

bool ZbbNopRule::shouldRun(const Context &) const {
    return RISCVAS.hasZbb() || RISCVAS.hasZbkb();
}

std::optional<Rule::Match> ZbbNopRule::match(ArrayRef<Instruction> Instructions,
                                             const Context &) const {
    using namespace LowLevelInstMatcherDSL;

    assert(Instructions.size() == 1 && "integer/zbb-nop requires one instruction");

    const MCInst &I = Instructions.front().Inst;

    switch (I.getOpcode()) {
    case RISCV::MIN:
    case RISCV::MINU:
    case RISCV::MAX:
    case RISCV::MAXU: {
        if (!RISCVAS.hasZbb()) // Zbkb does not provide min/max
            return std::nullopt;
        // min/max Rd, Rd, Rd: comparing a value with itself.
        Reg RdReg;
        if (matchInst(I, I.getOpcode(), RdReg, RdReg, RdReg) && RdReg.get() != RISCV::X0)
            return Match{};
        return std::nullopt;
    }
    case RISCV::ROL:
    case RISCV::ROR: {
        // rol/ror Rd, Rd, X0: rotating by zero.
        Reg RdReg;
        if (matchInst(I, I.getOpcode(), RdReg, RdReg, Reg(RISCV::X0)) && RdReg.get() != RISCV::X0)
            return Match{};
        return std::nullopt;
    }
    case RISCV::RORI: {
        // rori Rd, Rd, 0.
        Reg RdReg;
        if (matchInst(I, RISCV::RORI, RdReg, RdReg, Imm(0)) && RdReg.get() != RISCV::X0)
            return Match{};
        return std::nullopt;
    }
    case RISCV::ANDN: {
        // andn Rd, Rd, X0: rd & ~0.
        Reg RdReg;
        if (matchInst(I, RISCV::ANDN, RdReg, RdReg, Reg(RISCV::X0)) && RdReg.get() != RISCV::X0)
            return Match{};
        return std::nullopt;
    }
    default:
        return std::nullopt;
    }
}

} // namespace loonglint::RISCV
