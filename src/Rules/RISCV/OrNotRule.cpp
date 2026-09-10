// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/Rules/RISCV/OrNotRule.hpp"

#include "loonglint/MCInstMatcher.hpp"

#include "MCTargetDesc/RISCVMCTargetDesc.h"

#include "llvm/MC/MCInstBuilder.h"

#include <cassert>
#include <optional>

using namespace llvm;

namespace loonglint::RISCV {

namespace RISCV = ::llvm::RISCV;

OrNotRule::OrNotRule(const RISCVSpec &RISCVAS) : RISCVAS(RISCVAS) {}

StringRef OrNotRule::getID() const {
    return "riscv:integer/or-not";
}

StringRef OrNotRule::getDescription() const {
    return "fold 'xori -1; or' into 'orn'";
}

unsigned OrNotRule::getInstructionCount() const {
    return 2;
}

bool OrNotRule::shouldRun(const Context &) const {
    return RISCVAS.hasZbb() || RISCVAS.hasZbkb();
}

std::optional<Rule::Match> OrNotRule::match(ArrayRef<Instruction> Instructions,
                                            const Context &) const {
    using namespace LowLevelInstMatcherDSL;

    assert(Instructions.size() == 2 && "integer/or-not requires two instructions");

    const MCInst &F = Instructions[0].Inst;
    const MCInst &S = Instructions[1].Inst;

    // xori Tmp, Rs2, -1 (printed not); or Rd, Rs1, Tmp.
    Reg TmpReg, Rs2Reg;
    if (!matchInst(F, RISCV::XORI, TmpReg, Rs2Reg, Imm(-1)))
        return std::nullopt;
    const MCRegister Tmp = TmpReg.get();
    const MCRegister Rs2 = Rs2Reg.get();

    if (Tmp == RISCV::X0)
        return std::nullopt;

    Reg RdReg, Rs1Reg;
    // or is commutative; accept either operand order.
    if (!matchInst(S, RISCV::OR, RdReg, Rs1Reg, TmpReg) &&
        !matchInst(S, RISCV::OR, RdReg, TmpReg, Rs1Reg))
        return std::nullopt;

    // Same-destination chain. Rs1 must not alias the temporary the xori
    // clobbered, and the self-inverted form (Rs2 == Tmp) is the Zbs
    // mask-materialization middle owned by BclrRule.
    if (RdReg.get() != Tmp || Rs1Reg.get() == Tmp || Rs2 == Tmp)
        return std::nullopt;

    Rule::Match Result;
    Result.Replacement.emplace_back(
        MCInstBuilder(RISCV::ORN).addReg(Tmp).addReg(Rs1Reg.get()).addReg(Rs2));
    return Result;
}

} // namespace loonglint::RISCV
