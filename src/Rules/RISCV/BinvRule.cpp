// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/Rules/RISCV/BinvRule.hpp"

#include "loonglint/MCInstMatcher.hpp"

#include "MCTargetDesc/RISCVMCTargetDesc.h"

#include "llvm/MC/MCInstBuilder.h"

#include <cassert>

using namespace llvm;

namespace loonglint::RISCV {

namespace RISCV = ::llvm::RISCV;

BinvRule::BinvRule(const RISCVSpec &RISCVAS) : RISCVAS(RISCVAS) {}

StringRef BinvRule::getID() const {
    return "riscv:integer/binv";
}

StringRef BinvRule::getDescription() const {
    return "fuse bit-mask materialization with 'xor' into 'binv'";
}

unsigned BinvRule::getInstructionCount() const {
    return 3;
}

bool BinvRule::shouldRun(const Context &) const {
    return RISCVAS.hasZbs();
}

std::optional<Rule::Match> BinvRule::match(ArrayRef<Instruction> Instructions,
                                           const Context &) const {
    using namespace LowLevelInstMatcherDSL;

    assert(Instructions.size() == 3 && "integer/binv requires three instructions");

    const MCInst &F = Instructions[0].Inst;
    const MCInst &M = Instructions[1].Inst;
    const MCInst &S = Instructions[2].Inst;

    // addi Tmp, X0, 1 | c.li Tmp, 1; sll Tmp, Tmp, Rs2; xor Tmp, Rs1, Tmp.
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
    // xor is commutative; accept either operand order.
    if (!matchInst(S, RISCV::XOR, TmpReg, Rs1Reg, TmpReg) &&
        !matchInst(S, RISCV::XOR, TmpReg, TmpReg, Rs1Reg))
        return std::nullopt;

    if (Rs1Reg.get() == Tmp)
        return std::nullopt;

    Rule::Match Result;
    Result.Replacement.emplace_back(
        MCInstBuilder(RISCV::BINV).addReg(Tmp).addReg(Rs1Reg.get()).addReg(Rs2Reg.get()));
    return Result;
}

} // namespace loonglint::RISCV
