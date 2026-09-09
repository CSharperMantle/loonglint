// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/Rules/RISCV/ZbbAndnRule.hpp"

#include "loonglint/MCInstMatcher.hpp"

#include "MCTargetDesc/RISCVMCTargetDesc.h"

#include "llvm/MC/MCInstBuilder.h"

#include <cassert>
#include <optional>

using namespace llvm;

namespace loonglint::RISCV {

namespace RISCV = ::llvm::RISCV;

ZbbAndnRule::ZbbAndnRule(const RISCVSpec &RISCVAS) : RISCVAS(RISCVAS) {}

StringRef ZbbAndnRule::getID() const {
    return "riscv:integer/zbb-andn";
}

StringRef ZbbAndnRule::getDescription() const {
    return "fuse xori -1 + logic into andn/orn/xnor";
}

unsigned ZbbAndnRule::getInstructionCount() const {
    return 2;
}

bool ZbbAndnRule::shouldRun(const Context &) const {
    return RISCVAS.hasZbb() || RISCVAS.hasZbkb();
}

std::optional<Rule::Match> ZbbAndnRule::match(ArrayRef<Instruction> Instructions,
                                              const Context &) const {
    using namespace LowLevelInstMatcherDSL;

    assert(Instructions.size() == 2 && "integer/zbb-andn requires two instructions");

    const MCInst &F = Instructions[0].Inst;
    const MCInst &S = Instructions[1].Inst;

    // xori Tmp, Rs2, -1 (printed not); and/or/xor Rd, Rs1, Tmp.
    Reg TmpReg, Rs2Reg;
    if (!matchInst(F, RISCV::XORI, TmpReg, Rs2Reg, Imm(-1)))
        return std::nullopt;
    const MCRegister Tmp = TmpReg.get();
    const MCRegister Rs2 = Rs2Reg.get();

    if (Tmp == RISCV::X0)
        return std::nullopt;

    unsigned Op;
    Reg RdReg, Rs1Reg;
    // and/or/xor are commutative; accept either operand order.
    if (matchInst(S, RISCV::AND, RdReg, Rs1Reg, TmpReg) ||
        matchInst(S, RISCV::AND, RdReg, TmpReg, Rs1Reg))
        Op = RISCV::ANDN;
    else if (matchInst(S, RISCV::OR, RdReg, Rs1Reg, TmpReg) ||
             matchInst(S, RISCV::OR, RdReg, TmpReg, Rs1Reg))
        Op = RISCV::ORN;
    else if (matchInst(S, RISCV::XOR, RdReg, Rs1Reg, TmpReg) ||
             matchInst(S, RISCV::XOR, RdReg, TmpReg, Rs1Reg))
        Op = RISCV::XNOR;
    else
        return std::nullopt;

    // Same-destination chain. Rs1 must not alias the temporary the xori
    // clobbered, and the self-inverted form (Rs2 == Tmp) is the Zbs
    // mask-materialization middle owned by ZbsBclrRule.
    if (RdReg.get() != Tmp || Rs1Reg.get() == Tmp || Rs2 == Tmp)
        return std::nullopt;

    Rule::Match Result;
    Result.Replacement.emplace_back(MCInstBuilder(Op).addReg(Tmp).addReg(Rs1Reg.get()).addReg(Rs2));
    return Result;
}

} // namespace loonglint::RISCV
