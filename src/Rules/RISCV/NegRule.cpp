// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/Rules/RISCV/NegRule.hpp"

#include "loonglint/MCInstMatcher.hpp"

#include "MCTargetDesc/RISCVMCTargetDesc.h"

#include "llvm/MC/MCInstBuilder.h"

#include <cassert>

using namespace llvm;

namespace loonglint::RISCV {

namespace RISCV = ::llvm::RISCV;

StringRef NegRule::getID() const {
    return "riscv:integer/neg";
}

StringRef NegRule::getDescription() const {
    return "fuse not + addi 1 into sub";
}

unsigned NegRule::getInstructionCount() const {
    return 2;
}

std::optional<Rule::Match> NegRule::match(ArrayRef<Instruction> Instructions,
                                          const Context &) const {
    using namespace LowLevelInstMatcherDSL;

    assert(Instructions.size() == 2 && "integer/neg requires two instructions");

    const MCInst &F = Instructions[0].Inst;
    const MCInst &S = Instructions[1].Inst;

    // xori Rd, Rs, -1 (printed not); addi Rd, Rd, 1 -> sub Rd, X0, Rs.
    Reg RdReg, RsReg;
    if (!matchInst(F, RISCV::XORI, RdReg, RsReg, Imm(-1)))
        return std::nullopt;
    if (!matchInst(S, RISCV::ADDI, RdReg, RdReg, Imm(1)))
        return std::nullopt;
    const MCRegister Rd = RdReg.get();

    if (Rd == RISCV::X0)
        return std::nullopt;

    Rule::Match Result;
    Result.Replacement.emplace_back(
        MCInstBuilder(RISCV::SUB).addReg(Rd).addReg(RISCV::X0).addReg(RsReg.get()));
    return Result;
}

} // namespace loonglint::RISCV
