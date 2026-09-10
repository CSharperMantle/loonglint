// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/Rules/RISCV/NopZbkbRule.hpp"

#include "loonglint/MCInstMatcher.hpp"

#include "MCTargetDesc/RISCVMCTargetDesc.h"

#include <cassert>

using namespace llvm;

namespace loonglint::RISCV {

namespace RISCV = ::llvm::RISCV;

NopZbkbRule::NopZbkbRule(const RISCVSpec &RISCVAS) : RISCVAS(RISCVAS) {}

StringRef NopZbkbRule::getID() const {
    return "riscv:integer/nop-zbkb";
}

StringRef NopZbkbRule::getDescription() const {
    return "delete or replace non-canonical Zbkb nop instruction";
}

unsigned NopZbkbRule::getInstructionCount() const {
    return 1;
}

bool NopZbkbRule::shouldRun(const Context &) const {
    return RISCVAS.hasZbb() || RISCVAS.hasZbkb();
}

std::optional<Rule::Match> NopZbkbRule::match(ArrayRef<Instruction> Instructions,
                                              const Context &) const {
    using namespace LowLevelInstMatcherDSL;

    assert(Instructions.size() == 1 && "integer/nop-zbkb requires one instruction");

    const MCInst &I = Instructions.front().Inst;

    switch (I.getOpcode()) {
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
