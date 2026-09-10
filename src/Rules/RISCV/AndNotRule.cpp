// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/Rules/RISCV/AndNotRule.hpp"

#include "loonglint/MCInstMatcher.hpp"

#include "MCTargetDesc/RISCVMCTargetDesc.h"

#include "llvm/MC/MCInstBuilder.h"

#include <cassert>
#include <optional>

using namespace llvm;

namespace loonglint::RISCV {

namespace RISCV = ::llvm::RISCV;

AndNotRule::AndNotRule(const RISCVSpec &RISCVAS) : RISCVAS(RISCVAS) {}

StringRef AndNotRule::getID() const {
    return "riscv:integer/and-not";
}

StringRef AndNotRule::getDescription() const {
    return "fold 'xori -1; and' into 'andn'";
}

unsigned AndNotRule::getInstructionCount() const {
    return 2;
}

bool AndNotRule::shouldRun(const Context &) const {
    return RISCVAS.hasZbb() || RISCVAS.hasZbkb();
}

std::optional<Rule::Match> AndNotRule::match(ArrayRef<Instruction> Instructions,
                                             const Context &) const {
    using namespace LowLevelInstMatcherDSL;

    assert(Instructions.size() == 2 && "integer/and-not requires two instructions");

    const MCInst &F = Instructions[0].Inst;
    const MCInst &S = Instructions[1].Inst;

    // xori Tmp, Rs2, -1 (printed not); and Rd, Rs1, Tmp.
    Reg TmpReg, Rs2Reg;
    if (!matchInst(F, RISCV::XORI, TmpReg, Rs2Reg, Imm(-1)))
        return std::nullopt;
    const MCRegister Tmp = TmpReg.get();
    const MCRegister Rs2 = Rs2Reg.get();

    if (Tmp == RISCV::X0)
        return std::nullopt;

    Reg RdReg, Rs1Reg;
    // and is commutative; accept either operand order.
    if (!matchInst(S, RISCV::AND, RdReg, Rs1Reg, TmpReg) &&
        !matchInst(S, RISCV::AND, RdReg, TmpReg, Rs1Reg))
        return std::nullopt;

    // Same-destination chain. Rs1 must not alias the temporary the xori
    // clobbered, and the self-inverted form (Rs2 == Tmp) is the Zbs
    // mask-materialization middle owned by BclrRule.
    if (RdReg.get() != Tmp || Rs1Reg.get() == Tmp || Rs2 == Tmp)
        return std::nullopt;

    Rule::Match Result;
    Result.Replacement.emplace_back(
        MCInstBuilder(RISCV::ANDN).addReg(Tmp).addReg(Rs1Reg.get()).addReg(Rs2));
    return Result;
}

} // namespace loonglint::RISCV
