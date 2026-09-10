// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/Rules/RISCV/ZextWFormRule.hpp"

#include "loonglint/MCInstMatcher.hpp"

#include "MCTargetDesc/RISCVMCTargetDesc.h"

#include "llvm/MC/MCInstBuilder.h"

#include <cassert>

using namespace llvm;

namespace loonglint::RISCV {

namespace RISCV = ::llvm::RISCV;

ZextWFormRule::ZextWFormRule(const RISCVSpec &RISCVAS) : RISCVAS(RISCVAS) {}

StringRef ZextWFormRule::getID() const {
    return "riscv:integer/zext-w-form";
}

StringRef ZextWFormRule::getDescription() const {
    return "fold 'slli 32; srli 32' into 'zext.w'";
}

unsigned ZextWFormRule::getInstructionCount() const {
    return 2;
}

bool ZextWFormRule::shouldRun(const Context &) const {
    return RISCVAS.hasZba() && RISCVAS.isRV64();
}

std::optional<Rule::Match> ZextWFormRule::match(ArrayRef<Instruction> Instructions,
                                                const Context &) const {
    using namespace LowLevelInstMatcherDSL;

    assert(Instructions.size() == 2 && "integer/zext-w-form requires two instructions");

    const MCInst &F = Instructions[0].Inst;
    const MCInst &S = Instructions[1].Inst;

    // slli Rd, Rs, 32; srli Rd, Rd, 32 -> add.uw Rd, Rs, X0 (printed zext.w).
    Reg RdReg, RsReg;
    Imm ShamtImm;
    if (!matchInst(F, RISCV::SLLI, RdReg, RsReg, ShamtImm) || ShamtImm.get() != 32 ||
        !matchInst(S, RISCV::SRLI, RdReg, RdReg, Imm(32)) || RdReg.get() == RISCV::X0)
        return std::nullopt;

    Rule::Match Result;
    Result.Replacement.emplace_back(
        MCInstBuilder(RISCV::ADD_UW).addReg(RdReg.get()).addReg(RsReg.get()).addReg(RISCV::X0));
    return Result;
}

} // namespace loonglint::RISCV
