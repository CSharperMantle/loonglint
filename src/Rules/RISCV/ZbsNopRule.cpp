// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/Rules/RISCV/ZbsNopRule.hpp"

#include "loonglint/MCInstMatcher.hpp"

#include "MCTargetDesc/RISCVMCTargetDesc.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/MC/MCInstBuilder.h"

#include <cassert>

using namespace llvm;

namespace loonglint::RISCV {

namespace RISCV = ::llvm::RISCV;

ZbsNopRule::ZbsNopRule(const RISCVSpec &RISCVAS) : RISCVAS(RISCVAS) {}

StringRef ZbsNopRule::getID() const {
    return "riscv:integer/zbs-nop";
}

StringRef ZbsNopRule::getDescription() const {
    return "delete or simplify repeated single-bit operation";
}

unsigned ZbsNopRule::getInstructionCount() const {
    return 2;
}

bool ZbsNopRule::shouldRun(const Context &) const {
    return RISCVAS.hasZbs();
}

std::optional<Rule::Match> ZbsNopRule::match(ArrayRef<Instruction> Instructions,
                                             const Context &) const {
    using namespace LowLevelInstMatcherDSL;

    assert(Instructions.size() == 2 && "integer/zbs-nop requires two instructions");

    const MCInst &F = Instructions[0].Inst;
    const MCInst &S = Instructions[1].Inst;
    const unsigned Op = F.getOpcode();
    if (!is_contained({RISCV::BCLRI, RISCV::BSETI, RISCV::BINVI}, Op))
        return std::nullopt;

    Reg RdReg, RsReg;
    Imm ShamtImm;
    if (!matchInst(F, Op, RdReg, RsReg, ShamtImm))
        return std::nullopt;
    const MCRegister Rd = RdReg.get();

    if (Rd == RISCV::X0)
        return std::nullopt;

    // Same destination and same bit index: bclri/bseti are idempotent and
    // binvi is an involution.
    if (!matchInst(S, Op, RdReg, RdReg, ShamtImm))
        return std::nullopt;

    if (Op == RISCV::BINVI) {
        if (Rd == RsReg.get())
            return Match{};

        Rule::Match Result;
        Result.Replacement.emplace_back(
            MCInstBuilder(RISCV::ADDI).addReg(Rd).addReg(RsReg.get()).addImm(0));
        return Result;
    }

    Rule::Match Result;
    Result.Replacement.emplace_back(F);
    return Result;
}

} // namespace loonglint::RISCV
