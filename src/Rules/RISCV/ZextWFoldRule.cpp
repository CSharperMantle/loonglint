// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/Rules/RISCV/ZextWFoldRule.hpp"

#include "loonglint/MCInstMatcher.hpp"

#include "MCTargetDesc/RISCVMCTargetDesc.h"

#include "llvm/MC/MCInstBuilder.h"

#include <cassert>
#include <optional>

using namespace llvm;

namespace loonglint::RISCV {

namespace RISCV = ::llvm::RISCV;

ZextWFoldRule::ZextWFoldRule(const RISCVSpec &RISCVAS) : RISCVAS(RISCVAS) {}

StringRef ZextWFoldRule::getID() const {
    return "riscv:integer/zext-w-fold";
}

StringRef ZextWFoldRule::getDescription() const {
    return "fold 'zext.w' into a .uw consumer";
}

unsigned ZextWFoldRule::getInstructionCount() const {
    return 2;
}

bool ZextWFoldRule::shouldRun(const Context &) const {
    return RISCVAS.hasZba() && RISCVAS.isRV64();
}

std::optional<Rule::Match> ZextWFoldRule::match(ArrayRef<Instruction> Instructions,
                                                const Context &) const {
    using namespace LowLevelInstMatcherDSL;

    assert(Instructions.size() == 2 && "integer/zext-w-fold requires two instructions");

    const MCInst &F = Instructions[0].Inst;
    const MCInst &S = Instructions[1].Inst;

    // add.uw Tmp, Rs1, X0 feeding a consumer: fold the zero extension into the .uw forms.
    Reg TmpReg, Rs1Reg;
    if (!matchInst(F, RISCV::ADD_UW, TmpReg, Rs1Reg, Reg(RISCV::X0)) || TmpReg.get() == RISCV::X0)
        return std::nullopt;

    const MCRegister Tmp = TmpReg.get();

    // add Rd, Tmp, Rs2 (either operand order) -> add.uw Rd, Rs1, Rs2.
    do {
        Reg Rs2Reg;
        if (!matchInst(S, RISCV::ADD, TmpReg, TmpReg, Rs2Reg) &&
            !matchInst(S, RISCV::ADD, TmpReg, Rs2Reg, TmpReg))
            break;
        // Rs2 must not alias Tmp: the add reads it after add.uw wrote it.
        if (Rs2Reg.get() == Tmp)
            break;

        Rule::Match Result;
        Result.Replacement.emplace_back(
            MCInstBuilder(RISCV::ADD_UW).addReg(Tmp).addReg(Rs1Reg.get()).addReg(Rs2Reg.get()));
        return Result;
    } while (0);

    // shNadd Rd, Tmp, Rs2 -> shNadd.uw Rd, Rs1, Rs2.
    do {
        unsigned Op;
        Reg Rs2Reg;
        if (matchInst(S, RISCV::SH1ADD, TmpReg, TmpReg, Rs2Reg))
            Op = RISCV::SH1ADD_UW;
        else if (matchInst(S, RISCV::SH2ADD, TmpReg, TmpReg, Rs2Reg))
            Op = RISCV::SH2ADD_UW;
        else if (matchInst(S, RISCV::SH3ADD, TmpReg, TmpReg, Rs2Reg))
            Op = RISCV::SH3ADD_UW;
        else
            break;

        if (Rs2Reg.get() == Tmp)
            break;

        Rule::Match Result;
        Result.Replacement.emplace_back(
            MCInstBuilder(Op).addReg(Tmp).addReg(Rs1Reg.get()).addReg(Rs2Reg.get()));
        return Result;
    } while (0);

    // slli Rd, Tmp, N -> slli.uw Rd, Rs1, N. A zero shift is a nop owned by NopRule.
    do {
        Imm ShamtImm;
        if (!matchInst(S, RISCV::SLLI, TmpReg, TmpReg, ShamtImm) || ShamtImm.get() < 1)
            break;

        Rule::Match Result;
        Result.Replacement.emplace_back(
            MCInstBuilder(RISCV::SLLI_UW).addReg(Tmp).addReg(Rs1Reg.get()).addImm(ShamtImm.get()));
        return Result;
    } while (0);

    return std::nullopt;
}

} // namespace loonglint::RISCV
