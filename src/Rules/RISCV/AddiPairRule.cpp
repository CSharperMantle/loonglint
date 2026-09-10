// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/Rules/RISCV/AddiPairRule.hpp"

#include "loonglint/MCInstMatcher.hpp"

#include "MCTargetDesc/RISCVMCTargetDesc.h"

#include "llvm/MC/MCInstBuilder.h"
#include "llvm/Support/MathExtras.h"

#include <cassert>
#include <cstdint>

using namespace llvm;

namespace loonglint::RISCV {

namespace RISCV = ::llvm::RISCV;

StringRef AddiPairRule::getID() const {
    return "riscv:integer/addi-pair";
}

StringRef AddiPairRule::getDescription() const {
    return "fuse adjacent 'addi/c.addi' pair";
}

unsigned AddiPairRule::getInstructionCount() const {
    return 2;
}

std::optional<Rule::Match> AddiPairRule::match(ArrayRef<Instruction> Instructions,
                                               const Context &) const {
    using namespace LowLevelInstMatcherDSL;

    assert(Instructions.size() == 2 && "integer/addi-pair requires two instructions");

    const MCInst &F = Instructions[0].Inst;
    const MCInst &S = Instructions[1].Inst;

    // addi Rd, Rs, Imm0 | c.addi Rd, Imm0.
    bool FirstCompressed;
    Reg RdReg, RsReg;
    Imm FirstImm;
    if (matchInst(F, RISCV::ADDI, RdReg, RsReg, FirstImm)) {
        FirstCompressed = false;
    } else if (matchInst(F, RISCV::C_ADDI, RdReg, RdReg, FirstImm))
        FirstCompressed = true;
    else
        return std::nullopt;
    const MCRegister Rd = RdReg.get();
    const MCRegister Rs = FirstCompressed ? Rd : RsReg.get();
    const int64_t First = FirstImm.get();

    if (Rd == RISCV::X0)
        return std::nullopt;
    if (First == 0) // an addi-by-0 is a NOP only when it rewrites its own source
        return std::nullopt;

    // addi Rd, Rd, Imm1 | c.addi Rd, Imm1: the second overwrites the chain.
    Imm SecondImm;
    if (!matchInst(S, RISCV::ADDI, RdReg, RdReg, SecondImm) &&
        !matchInst(S, RISCV::C_ADDI, RdReg, RdReg, SecondImm))
        return std::nullopt;
    const int64_t Second = SecondImm.get();

    if (Second == 0)
        return std::nullopt;

    const int64_t Combined = First + Second;
    if (!isInt<12>(Combined))
        return std::nullopt;

    if (Combined == 0) {
        // The pair leaves Rd = Rs: delete the NOP pair, or copy Rs.
        if (Rd == Rs)
            return Match{};
    }

    Rule::Match Result;
    Result.Replacement.emplace_back(
        MCInstBuilder(RISCV::ADDI).addReg(Rd).addReg(Rs).addImm(Combined));
    return Result;
}

} // namespace loonglint::RISCV
