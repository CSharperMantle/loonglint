// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/Rules/RISCV/ZbaShAddRule.hpp"

#include "loonglint/MCInstMatcher.hpp"

#include "MCTargetDesc/RISCVMCTargetDesc.h"

#include "llvm/MC/MCInstBuilder.h"

#include <cassert>
#include <cstdint>

using namespace llvm;

namespace loonglint::RISCV {

namespace RISCV = ::llvm::RISCV;

ZbaShAddRule::ZbaShAddRule(const RISCVSpec &RISCVAS) : RISCVAS(RISCVAS) {}

StringRef ZbaShAddRule::getID() const {
    return "riscv:integer/zba-sh-add";
}

StringRef ZbaShAddRule::getDescription() const {
    return "fuse 'slli; add' into Zba 'shNadd'";
}

unsigned ZbaShAddRule::getInstructionCount() const {
    return 2;
}

bool ZbaShAddRule::shouldRun(const Context &) const {
    return RISCVAS.hasZba();
}

std::optional<Rule::Match> ZbaShAddRule::match(ArrayRef<Instruction> Instructions,
                                               const Context &) const {
    using namespace LowLevelInstMatcherDSL;

    assert(Instructions.size() == 2 && "integer/zba-sh-add requires two instructions");

    const MCInst &F = Instructions[0].Inst;
    const MCInst &S = Instructions[1].Inst;

    // slli Rd, Rs1, N; add Rd, Rd, Rs2 -> shNadd Rd, Rs1, Rs2.
    Reg RdReg, Rs1Reg;
    Imm ShamtImm;
    if (!matchInst(F, RISCV::SLLI, RdReg, Rs1Reg, ShamtImm))
        return std::nullopt;
    const int64_t Shamt = ShamtImm.get();
    const MCRegister Rd = RdReg.get();

    if (Shamt < 1 || Shamt > 3 || Rd == RISCV::X0)
        return std::nullopt;

    Reg Rs2Reg;
    // add is commutative; accept either operand order.
    if (!matchInst(S, RISCV::ADD, RdReg, RdReg, Rs2Reg) &&
        !matchInst(S, RISCV::ADD, RdReg, Rs2Reg, RdReg))
        return std::nullopt;
    // Rs2 must not alias Rd: the slli overwrites it before the add reads it.
    if (Rs2Reg.get() == Rd)
        return std::nullopt;

    const unsigned Op = Shamt == 1 ? RISCV::SH1ADD : Shamt == 2 ? RISCV::SH2ADD : RISCV::SH3ADD;

    Rule::Match Result;
    Result.Replacement.emplace_back(
        MCInstBuilder(Op).addReg(Rd).addReg(Rs1Reg.get()).addReg(Rs2Reg.get()));
    return Result;
}

} // namespace loonglint::RISCV
