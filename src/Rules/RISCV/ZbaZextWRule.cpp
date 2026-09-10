// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/Rules/RISCV/ZbaZextWRule.hpp"

#include "loonglint/MCInstMatcher.hpp"

#include "MCTargetDesc/RISCVMCTargetDesc.h"

#include "llvm/MC/MCInstBuilder.h"

#include <cassert>
#include <optional>

using namespace llvm;

namespace loonglint::RISCV {

namespace RISCV = ::llvm::RISCV;

// Returns true when |I| writes a value that statically fits in 32 bits to |Rd|.
static bool isZextWProducer(const MCInst &I, MCRegister Rd) {
    using namespace LowLevelInstMatcherDSL;

    Reg RdReg(Rd);
    switch (I.getOpcode()) {
    case RISCV::LWU:
    case RISCV::LBU:
    case RISCV::LHU:
    case RISCV::ZEXT_H_RV64:
        // Only the destination matters; the other operands do not affect the
        // result's width.
        return matchInst(I, I.getOpcode(), RdReg);
    case RISCV::ADD_UW:
        // zext32(Rs1) + Rs2 only stays within 32 bits when the addend is X0.
        return matchInst(I, RISCV::ADD_UW, RdReg, Reg(), Reg(RISCV::X0));
    case RISCV::ANDI: {
        Imm ImmValue;
        return matchInst(I, RISCV::ANDI, RdReg, Reg(), ImmValue) && ImmValue.get() >= 0;
    }
    case RISCV::SRLI: {
        Imm ShamtImm;
        return matchInst(I, RISCV::SRLI, RdReg, Reg(), ShamtImm) && ShamtImm.get() >= 32;
    }
    default:
        return false;
    }
}

ZbaZextWRule::ZbaZextWRule(const RISCVSpec &RISCVAS) : RISCVAS(RISCVAS) {}

StringRef ZbaZextWRule::getID() const {
    return "riscv:integer/zba-zext-w";
}

StringRef ZbaZextWRule::getDescription() const {
    return "fuse word-sized zero extension into Zba 'zext.w'";
}

unsigned ZbaZextWRule::getInstructionCount() const {
    return 2;
}

bool ZbaZextWRule::shouldRun(const Context &) const {
    return RISCVAS.hasZba() && RISCVAS.isRV64();
}

std::optional<Rule::Match> ZbaZextWRule::match(ArrayRef<Instruction> Instructions,
                                               const Context &) const {
    using namespace LowLevelInstMatcherDSL;

    assert(Instructions.size() == 2 && "integer/zba-zext-w requires two instructions");

    const MCInst &F = Instructions[0].Inst;
    const MCInst &S = Instructions[1].Inst;

    // slli Rd, Rs, 32; srli Rd, Rd, 32 -> add.uw Rd, Rs, X0 (printed zext.w).
    do {
        Reg RdReg, RsReg;
        Imm ShamtImm;
        if (!matchInst(F, RISCV::SLLI, RdReg, RsReg, ShamtImm) || ShamtImm.get() != 32 ||
            !matchInst(S, RISCV::SRLI, RdReg, RdReg, Imm(32)) || RdReg.get() == RISCV::X0)
            break;

        Rule::Match Result;
        Result.Replacement.emplace_back(
            MCInstBuilder(RISCV::ADD_UW).addReg(RdReg.get()).addReg(RsReg.get()).addReg(RISCV::X0));
        return Result;
    } while (0);

    // producer; add.uw Rd, Rd, X0 -> delete the redundant zext.w.
    do {
        Reg RdReg;
        if (!matchInst(S, RISCV::ADD_UW, RdReg, RdReg, Reg(RISCV::X0)) ||
            RdReg.get() == RISCV::X0 || !isZextWProducer(F, RdReg.get()))
            break;

        Rule::Match Result;
        Result.Replacement.emplace_back(F);
        return Result;
    } while (0);

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

    // slli Rd, Tmp, N -> slli.uw Rd, Rs1, N. A zero shift is a NOP owned by NopRule.
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
