// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/Rules/RISCV/ZextHFormRule.hpp"

#include "loonglint/MCInstMatcher.hpp"

#include "MCTargetDesc/RISCVMCTargetDesc.h"

#include "llvm/MC/MCInstBuilder.h"

#include <cassert>
#include <cstdint>

using namespace llvm;

namespace loonglint::RISCV {

namespace RISCV = ::llvm::RISCV;

ZextHFormRule::ZextHFormRule(const RISCVSpec &RISCVAS) : RISCVAS(RISCVAS) {}

StringRef ZextHFormRule::getID() const {
    return "riscv:integer/zext-h-form";
}

StringRef ZextHFormRule::getDescription() const {
    return "fold shift pair into 'zext.h'";
}

unsigned ZextHFormRule::getInstructionCount() const {
    return 2;
}

bool ZextHFormRule::shouldRun(const Context &) const {
    return RISCVAS.hasZbb() || RISCVAS.hasZbkb();
}

std::optional<Rule::Match> ZextHFormRule::match(ArrayRef<Instruction> Instructions,
                                                const Context &) const {
    using namespace LowLevelInstMatcherDSL;

    assert(Instructions.size() == 2 && "integer/zext-h-form requires two instructions");

    const MCInst &F = Instructions[0].Inst;
    const MCInst &S = Instructions[1].Inst;
    const int64_t XLEN = RISCVAS.isRV64() ? 64 : 32;
    const unsigned ZextHOp = RISCVAS.isRV64() ? RISCV::ZEXT_H_RV64 : RISCV::ZEXT_H_RV32;

    // slli Rd, Rs, XLEN-16; srli Rd, Rd, XLEN-16 -> zext.h Rd, Rs.
    Reg RdReg, RsReg;
    Imm ShamtImm;
    if (!matchInst(F, RISCV::SLLI, RdReg, RsReg, ShamtImm) || ShamtImm.get() != XLEN - 16 ||
        !matchInst(S, RISCV::SRLI, RdReg, RdReg, Imm(XLEN - 16)) || RdReg.get() == RISCV::X0)
        return std::nullopt;

    Rule::Match Result;
    Result.Replacement.emplace_back(MCInstBuilder(ZextHOp).addReg(RdReg.get()).addReg(RsReg.get()));
    return Result;
}

} // namespace loonglint::RISCV
