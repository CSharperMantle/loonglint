// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/Rules/RISCV/ZextWElimRule.hpp"

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

ZextWElimRule::ZextWElimRule(const RISCVSpec &RISCVAS) : RISCVAS(RISCVAS) {}

StringRef ZextWElimRule::getID() const {
    return "riscv:integer/zext-w-elim";
}

StringRef ZextWElimRule::getDescription() const {
    return "delete redundant 'zext.w'";
}

unsigned ZextWElimRule::getInstructionCount() const {
    return 2;
}

bool ZextWElimRule::shouldRun(const Context &) const {
    return RISCVAS.hasZba() && RISCVAS.isRV64();
}

std::optional<Rule::Match> ZextWElimRule::match(ArrayRef<Instruction> Instructions,
                                                const Context &) const {
    using namespace LowLevelInstMatcherDSL;

    assert(Instructions.size() == 2 && "integer/zext-w-elim requires two instructions");

    const MCInst &F = Instructions[0].Inst;
    const MCInst &S = Instructions[1].Inst;

    // producer; add.uw Rd, Rd, X0 -> delete the redundant zext.w.
    Reg RdReg;
    if (!matchInst(S, RISCV::ADD_UW, RdReg, RdReg, Reg(RISCV::X0)) || RdReg.get() == RISCV::X0 ||
        !isZextWProducer(F, RdReg.get()))
        return std::nullopt;

    Rule::Match Result;
    Result.Replacement.emplace_back(F);
    return Result;
}

} // namespace loonglint::RISCV
