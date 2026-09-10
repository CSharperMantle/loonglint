// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/Rules/RISCV/SextFormRule.hpp"

#include "loonglint/MCInstMatcher.hpp"

#include "MCTargetDesc/RISCVMCTargetDesc.h"

#include "llvm/MC/MCInstBuilder.h"

#include <cassert>
#include <cstdint>

using namespace llvm;

namespace loonglint::RISCV {

namespace RISCV = ::llvm::RISCV;

SextFormRule::SextFormRule(const RISCVSpec &RISCVAS) : RISCVAS(RISCVAS) {}

StringRef SextFormRule::getID() const {
    return "riscv:integer/sext-form";
}

StringRef SextFormRule::getDescription() const {
    return "fold shift pair into 'sext.b/sext.h'";
}

unsigned SextFormRule::getInstructionCount() const {
    return 2;
}

bool SextFormRule::shouldRun(const Context &) const {
    return RISCVAS.hasZbb();
}

std::optional<Rule::Match> SextFormRule::match(ArrayRef<Instruction> Instructions,
                                               const Context &) const {
    using namespace LowLevelInstMatcherDSL;

    assert(Instructions.size() == 2 && "integer/sext-form requires two instructions");

    const MCInst &F = Instructions[0].Inst;
    const MCInst &S = Instructions[1].Inst;
    const int64_t XLEN = RISCVAS.isRV64() ? 64 : 32;

    // slli Rd, Rs, XLEN-16/8; srai Rd, Rd, XLEN-16/8 -> sext.h/sext.b.
    Reg RdReg, RsReg;
    Imm ShamtImm;
    if (!matchInst(F, RISCV::SLLI, RdReg, RsReg, ShamtImm))
        return std::nullopt;
    const int64_t Shamt = ShamtImm.get();

    unsigned Op = 0;
    if (Shamt == XLEN - 16)
        Op = RISCV::SEXT_H;
    else if (Shamt == XLEN - 8)
        Op = RISCV::SEXT_B;
    else
        return std::nullopt;

    if (RdReg.get() == RISCV::X0 || !matchInst(S, RISCV::SRAI, RdReg, RdReg, Imm(Shamt)))
        return std::nullopt;

    Rule::Match Result;
    Result.Replacement.emplace_back(MCInstBuilder(Op).addReg(RdReg.get()).addReg(RsReg.get()));
    return Result;
}

} // namespace loonglint::RISCV
