// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/Rules/RISCV/NopZbbRule.hpp"

#include "loonglint/MCInstMatcher.hpp"

#include "MCTargetDesc/RISCVMCTargetDesc.h"

#include <cassert>

using namespace llvm;

namespace loonglint::RISCV {

namespace RISCV = ::llvm::RISCV;

NopZbbRule::NopZbbRule(const RISCVSpec &RISCVAS) : RISCVAS(RISCVAS) {}

StringRef NopZbbRule::getID() const {
    return "riscv:integer/nop-zbb";
}

StringRef NopZbbRule::getDescription() const {
    return "delete or replace non-canonical Zbb nop instruction";
}

unsigned NopZbbRule::getInstructionCount() const {
    return 1;
}

bool NopZbbRule::shouldRun(const Context &) const {
    return RISCVAS.hasZbb();
}

std::optional<Rule::Match> NopZbbRule::match(ArrayRef<Instruction> Instructions,
                                             const Context &) const {
    using namespace LowLevelInstMatcherDSL;

    assert(Instructions.size() == 1 && "integer/nop-zbb requires one instruction");

    const MCInst &I = Instructions.front().Inst;

    switch (I.getOpcode()) {
    case RISCV::MIN:
    case RISCV::MINU:
    case RISCV::MAX:
    case RISCV::MAXU: {
        // min/max Rd, Rd, Rd: comparing a value with itself.
        Reg RdReg;
        if (matchInst(I, I.getOpcode(), RdReg, RdReg, RdReg) && RdReg.get() != RISCV::X0)
            return Match{};
        return std::nullopt;
    }
    default:
        return std::nullopt;
    }
}

} // namespace loonglint::RISCV
