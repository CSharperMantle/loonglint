// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/Rules/RISCV/BsetRule.hpp"

#include "loonglint/MCInstMatcher.hpp"

#include "MCTargetDesc/RISCVMCTargetDesc.h"

#include "llvm/MC/MCInstBuilder.h"

#include <cassert>

using namespace llvm;

namespace loonglint::RISCV {

namespace RISCV = ::llvm::RISCV;

BsetRule::BsetRule(const RISCVSpec &RISCVAS) : RISCVAS(RISCVAS) {}

StringRef BsetRule::getID() const {
    return "riscv:integer/bset";
}

StringRef BsetRule::getDescription() const {
    return "fuse bit-mask materialization with 'or' into 'bset'";
}

unsigned BsetRule::getInstructionCount() const {
    return 3;
}

bool BsetRule::shouldRun(const Context &) const {
    return RISCVAS.hasZbs();
}

std::optional<Rule::Match> BsetRule::match(ArrayRef<Instruction> Instructions,
                                           const Context &) const {
    using namespace LowLevelInstMatcherDSL;

    assert(Instructions.size() == 3 && "integer/bset requires three instructions");

    const MCInst &F = Instructions[0].Inst;
    const MCInst &M = Instructions[1].Inst;
    const MCInst &S = Instructions[2].Inst;

    // addi Tmp, X0, 1 | c.li Tmp, 1; sll Tmp, Tmp, Rs2; or Tmp, Rs1, Tmp.
    Reg TmpReg;
    if (!matchInst(F, RISCV::ADDI, TmpReg, Reg(RISCV::X0), Imm(1)) &&
        !matchInst(F, RISCV::C_LI, TmpReg, Imm(1)))
        return std::nullopt;
    const MCRegister Tmp = TmpReg.get();

    if (Tmp == RISCV::X0)
        return std::nullopt;

    Reg Rs2Reg;
    if (!matchInst(M, RISCV::SLL, TmpReg, TmpReg, Rs2Reg))
        return std::nullopt;
    if (Rs2Reg.get() == Tmp)
        return std::nullopt;

    Reg Rs1Reg;
    // or is commutative; accept either operand order.
    if (!matchInst(S, RISCV::OR, TmpReg, Rs1Reg, TmpReg) &&
        !matchInst(S, RISCV::OR, TmpReg, TmpReg, Rs1Reg))
        return std::nullopt;

    if (Rs1Reg.get() == Tmp)
        return std::nullopt;

    Rule::Match Result;
    Result.Replacement.emplace_back(
        MCInstBuilder(RISCV::BSET).addReg(Tmp).addReg(Rs1Reg.get()).addReg(Rs2Reg.get()));
    return Result;
}

} // namespace loonglint::RISCV
