// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/Rules/RISCV/ZbaNopRule.hpp"

#include "loonglint/MCInstMatcher.hpp"

#include "MCTargetDesc/RISCVMCTargetDesc.h"

#include <cassert>

using namespace llvm;

namespace loonglint::RISCV {

namespace RISCV = ::llvm::RISCV;

ZbaNopRule::ZbaNopRule(const RISCVSpec &RISCVAS) : RISCVAS(RISCVAS) {}

StringRef ZbaNopRule::getID() const {
    return "riscv:integer/zba-nop";
}

StringRef ZbaNopRule::getDescription() const {
    return "delete Zba NOP instruction";
}

unsigned ZbaNopRule::getInstructionCount() const {
    return 1;
}

bool ZbaNopRule::shouldRun(const Context &) const {
    return RISCVAS.hasZba();
}

std::optional<Rule::Match> ZbaNopRule::match(ArrayRef<Instruction> Instructions,
                                             const Context &) const {
    using namespace LowLevelInstMatcherDSL;

    assert(Instructions.size() == 1 && "integer/zba-nop requires one instruction");

    const MCInst &I = Instructions.front().Inst;

    switch (I.getOpcode()) {
    case RISCV::SH1ADD:
    case RISCV::SH2ADD:
    case RISCV::SH3ADD:
    case RISCV::ADD_UW:
    case RISCV::SH1ADD_UW:
    case RISCV::SH2ADD_UW:
    case RISCV::SH3ADD_UW: {
        // shNadd/shNadd.uw/add.uw Rd, X0, Rd: (0 << N) + Rd = Rd and
        // zext32(0) + Rd = Rd.
        Reg RdReg;
        if (matchInst(I, I.getOpcode(), RdReg, Reg(RISCV::X0), RdReg) && RdReg.get() != RISCV::X0)
            return Match{};
        return std::nullopt;
    }
    default:
        return std::nullopt;
    }
}

} // namespace loonglint::RISCV
