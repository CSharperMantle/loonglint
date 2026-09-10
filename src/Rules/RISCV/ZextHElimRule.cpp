// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/Rules/RISCV/ZextHElimRule.hpp"

#include "loonglint/MCInstMatcher.hpp"

#include "MCTargetDesc/RISCVMCTargetDesc.h"

#include "llvm/MC/MCInstBuilder.h"

#include <cassert>
#include <cstdint>

using namespace llvm;

namespace loonglint::RISCV {

namespace RISCV = ::llvm::RISCV;

// Returns true when |I| writes a value that statically fits in 16 bits to |Rd|.
static bool isZextHProducer(const MCInst &I, MCRegister Rd, bool IsRV64) {
    using namespace LowLevelInstMatcherDSL;

    Reg RdReg(Rd);
    switch (I.getOpcode()) {
    case RISCV::LBU:
    case RISCV::LHU:
    case RISCV::ZEXT_H_RV32:
    case RISCV::ZEXT_H_RV64:
        // Only the destination matters; the other operands do not affect the
        // result's width.
        return matchInst(I, I.getOpcode(), RdReg);
    case RISCV::ANDI: {
        Imm ImmValue;
        return matchInst(I, RISCV::ANDI, RdReg, Reg(), ImmValue) && ImmValue.get() >= 0;
    }
    case RISCV::SRLI: {
        Imm ShamtImm;
        return matchInst(I, RISCV::SRLI, RdReg, Reg(), ShamtImm) &&
               ShamtImm.get() >= (IsRV64 ? 48 : 16);
    }
    default:
        return false;
    }
}

ZextHElimRule::ZextHElimRule(const RISCVSpec &RISCVAS) : RISCVAS(RISCVAS) {}

StringRef ZextHElimRule::getID() const {
    return "riscv:integer/zext-h-elim";
}

StringRef ZextHElimRule::getDescription() const {
    return "delete redundant 'zext.h'";
}

unsigned ZextHElimRule::getInstructionCount() const {
    return 2;
}

bool ZextHElimRule::shouldRun(const Context &) const {
    return RISCVAS.hasZbb() || RISCVAS.hasZbkb();
}

std::optional<Rule::Match> ZextHElimRule::match(ArrayRef<Instruction> Instructions,
                                                const Context &) const {
    using namespace LowLevelInstMatcherDSL;

    assert(Instructions.size() == 2 && "integer/zext-h-elim requires two instructions");

    const MCInst &F = Instructions[0].Inst;
    const MCInst &S = Instructions[1].Inst;
    const bool IsRV64 = RISCVAS.isRV64();
    const unsigned ZextHOp = IsRV64 ? RISCV::ZEXT_H_RV64 : RISCV::ZEXT_H_RV32;

    // producer; zext.h Rd, Rd -> delete the redundant extension.
    Reg RdReg;
    if (!matchInst(S, ZextHOp, RdReg, RdReg) || RdReg.get() == RISCV::X0 ||
        !isZextHProducer(F, RdReg.get(), IsRV64))
        return std::nullopt;

    Rule::Match Result;
    Result.Replacement.emplace_back(F);
    return Result;
}

} // namespace loonglint::RISCV
