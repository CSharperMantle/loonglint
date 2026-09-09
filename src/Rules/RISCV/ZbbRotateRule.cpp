// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/Rules/RISCV/ZbbRotateRule.hpp"

#include "loonglint/MCInstMatcher.hpp"

#include "MCTargetDesc/RISCVMCTargetDesc.h"

#include "llvm/MC/MCInstBuilder.h"

#include <cassert>
#include <cstdint>

using namespace llvm;

namespace loonglint::RISCV {

namespace RISCV = ::llvm::RISCV;

ZbbRotateRule::ZbbRotateRule(const RISCVSpec &RISCVAS) : RISCVAS(RISCVAS) {}

StringRef ZbbRotateRule::getID() const {
    return "riscv:integer/zbb-rotate";
}

StringRef ZbbRotateRule::getDescription() const {
    return "fuse adjacent rotations";
}

unsigned ZbbRotateRule::getInstructionCount() const {
    return 2;
}

bool ZbbRotateRule::shouldRun(const Context &) const {
    return RISCVAS.hasZbb() || RISCVAS.hasZbkb();
}

std::optional<Rule::Match> ZbbRotateRule::match(ArrayRef<Instruction> Instructions,
                                                const Context &) const {
    using namespace LowLevelInstMatcherDSL;

    assert(Instructions.size() == 2 && "integer/zbb-rotate requires two instructions");

    const MCInst &F = Instructions[0].Inst;
    const MCInst &S = Instructions[1].Inst;
    const int64_t XLEN = RISCVAS.isRV64() ? 64 : 32;

    // rori Rd, Rs, Shamt0; rori Rd, Rd, Shamt1.
    Reg RdReg, RsReg;
    Imm FirstImm;
    if (!matchInst(F, RISCV::RORI, RdReg, RsReg, FirstImm))
        return std::nullopt;
    Imm SecondImm;
    if (!matchInst(S, RISCV::RORI, RdReg, RdReg, SecondImm))
        return std::nullopt;
    const MCRegister Rd = RdReg.get();
    const MCRegister Rs = RsReg.get();
    const int64_t First = FirstImm.get();
    const int64_t Second = SecondImm.get();

    if (Rd == RISCV::X0 || First < 1 || Second < 1)
        return std::nullopt;

    const int64_t Combined = (First + Second) % XLEN;
    if (Combined == 0) {
        // The pair rotates by a full turn: delete it or copy Rs.
        if (Rd == Rs)
            return Match{};

        Rule::Match Result;
        Result.Replacement.emplace_back(MCInstBuilder(RISCV::ADDI).addReg(Rd).addReg(Rs).addImm(0));
        return Result;
    }

    Rule::Match Result;
    Result.Replacement.emplace_back(
        MCInstBuilder(RISCV::RORI).addReg(Rd).addReg(Rs).addImm(Combined));
    return Result;
}

} // namespace loonglint::RISCV
