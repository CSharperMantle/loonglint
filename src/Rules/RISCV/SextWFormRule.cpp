// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/Rules/RISCV/SextWFormRule.hpp"

#include "loonglint/MCInstMatcher.hpp"

#include "MCTargetDesc/RISCVMCTargetDesc.h"

#include "llvm/MC/MCInstBuilder.h"

#include <cassert>

using namespace llvm;

namespace loonglint::RISCV {

namespace RISCV = ::llvm::RISCV;

SextWFormRule::SextWFormRule(const RISCVSpec &RISCVAS) : RISCVAS(RISCVAS) {}

StringRef SextWFormRule::getID() const {
    return "riscv:integer/sext-w-form";
}

StringRef SextWFormRule::getDescription() const {
    return "fold 'slli 32; srai 32' into 'sext.w'";
}

unsigned SextWFormRule::getInstructionCount() const {
    return 2;
}

bool SextWFormRule::shouldRun(const Context &) const {
    return RISCVAS.isRV64();
}

std::optional<Rule::Match> SextWFormRule::match(ArrayRef<Instruction> Instructions,
                                                const Context &) const {
    using namespace LowLevelInstMatcherDSL;

    assert(Instructions.size() == 2 && "integer/sext-w-form requires two instructions");

    const MCInst &F = Instructions[0].Inst;
    const MCInst &S = Instructions[1].Inst;

    // slli Rd, Rs, 32; srai Rd, Rd, 32 -> addiw Rd, Rs, 0.
    Reg RdReg, RsReg;
    Imm ShamtImm;
    if (!matchInst(F, RISCV::SLLI, RdReg, RsReg, ShamtImm) || ShamtImm.get() != 32)
        return std::nullopt;
    if (!matchInst(S, RISCV::SRAI, RdReg, RdReg, Imm(32)))
        return std::nullopt;

    const MCRegister Rd = RdReg.get();
    if (Rd == RISCV::X0)
        return std::nullopt;

    Rule::Match Result;
    Result.Replacement.emplace_back(
        MCInstBuilder(RISCV::ADDIW).addReg(Rd).addReg(RsReg.get()).addImm(0));
    return Result;
}

} // namespace loonglint::RISCV
