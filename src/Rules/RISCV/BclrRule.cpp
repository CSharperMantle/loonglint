// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/Rules/RISCV/BclrRule.hpp"

#include "loonglint/MCInstMatcher.hpp"

#include "MCTargetDesc/RISCVMCTargetDesc.h"

#include "llvm/MC/MCInstBuilder.h"

#include <cassert>

using namespace llvm;

namespace loonglint::RISCV {

namespace RISCV = ::llvm::RISCV;

BclrRule::BclrRule(const RISCVSpec &RISCVAS) : RISCVAS(RISCVAS) {}

StringRef BclrRule::getID() const {
    return "riscv:integer/bclr";
}

StringRef BclrRule::getDescription() const {
    return "fuse inverted bit-mask materialization with 'and' into 'bclr'";
}

unsigned BclrRule::getInstructionCount() const {
    return 4;
}

bool BclrRule::shouldRun(const Context &) const {
    return RISCVAS.hasZbs();
}

std::optional<Rule::Match> BclrRule::match(ArrayRef<Instruction> Instructions,
                                           const Context &) const {
    using namespace LowLevelInstMatcherDSL;

    assert(Instructions.size() == 4 && "integer/bclr requires four instructions");

    const MCInst &I0 = Instructions[0].Inst;
    const MCInst &I1 = Instructions[1].Inst;
    const MCInst &I2 = Instructions[2].Inst;
    const MCInst &I3 = Instructions[3].Inst;

    // addi Tmp, X0, 1 | c.li Tmp, 1; sll Tmp, Tmp, Rs2; xori Tmp, Tmp, -1;
    // and Tmp, Rs1, Tmp -> bclr Tmp, Rs1, Rs2.
    Reg TmpReg;
    if (!matchInst(I0, RISCV::ADDI, TmpReg, Reg(RISCV::X0), Imm(1)) &&
        !matchInst(I0, RISCV::C_LI, TmpReg, Imm(1)))
        return std::nullopt;
    const MCRegister Tmp = TmpReg.get();

    if (Tmp == RISCV::X0)
        return std::nullopt;

    Reg Rs2Reg;
    if (!matchInst(I1, RISCV::SLL, TmpReg, TmpReg, Rs2Reg) || Rs2Reg.get() == Tmp)
        return std::nullopt;
    if (!matchInst(I2, RISCV::XORI, TmpReg, TmpReg, Imm(-1)))
        return std::nullopt;

    Reg Rs1Reg;
    // and is commutative; accept either operand order.
    if (!matchInst(I3, RISCV::AND, TmpReg, Rs1Reg, TmpReg) &&
        !matchInst(I3, RISCV::AND, TmpReg, TmpReg, Rs1Reg))
        return std::nullopt;
    if (Rs1Reg.get() == Tmp)
        return std::nullopt;

    Rule::Match Result;
    Result.Replacement.emplace_back(
        MCInstBuilder(RISCV::BCLR).addReg(Tmp).addReg(Rs1Reg.get()).addReg(Rs2Reg.get()));
    return Result;
}

} // namespace loonglint::RISCV
