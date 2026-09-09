// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/Rules/RISCV/LogicImmediateRule.hpp"

#include "loonglint/MCInstMatcher.hpp"

#include "MCTargetDesc/RISCVMCTargetDesc.h"

#include "llvm/MC/MCInstBuilder.h"
#include "llvm/Support/ErrorHandling.h"

#include <cassert>
#include <cstdint>

using namespace llvm;

namespace loonglint::RISCV {

namespace RISCV = ::llvm::RISCV;

StringRef LogicImmediateRule::getID() const {
    return "riscv:integer/logic-immediate";
}

StringRef LogicImmediateRule::getDescription() const {
    return "fuse adjacent logic-immediate pair";
}

unsigned LogicImmediateRule::getInstructionCount() const {
    return 2;
}

std::optional<Rule::Match> LogicImmediateRule::match(ArrayRef<Instruction> Instructions,
                                                     const Context &) const {
    using namespace LowLevelInstMatcherDSL;

    assert(Instructions.size() == 2 && "integer/logic-immediate requires two instructions");

    const MCInst &F = Instructions[0].Inst;
    const MCInst &S = Instructions[1].Inst;
    const unsigned Op = F.getOpcode();
    if (Op != RISCV::ORI && Op != RISCV::XORI && Op != RISCV::ANDI)
        return std::nullopt;

    Reg RdReg, RsReg;
    Imm FirstImm;
    if (!matchInst(F, Op, RdReg, RsReg, FirstImm))
        return std::nullopt;
    const MCRegister Rd = RdReg.get();
    if (Rd == RISCV::X0)
        return std::nullopt;
    const int64_t First = FirstImm.get();
    if (First == 0) // zero immediates are not pair members
        return std::nullopt;

    Imm SecondImm;
    if (!matchInst(S, Op, RdReg, RdReg, SecondImm))
        return std::nullopt;
    const int64_t Second = SecondImm.get();
    if (Second == 0)
        return std::nullopt;

    // The or/xor/and of two sign-extended 12-bit values is always a
    // sign-extended 12-bit value, so the combined immediate is encodable.
    int64_t Combined;
    switch (Op) {
    case RISCV::ORI:
        Combined = First | Second;
        break;
    case RISCV::XORI:
        Combined = First ^ Second;
        break;
    case RISCV::ANDI:
        Combined = First & Second;
        break;
    default:
        llvm_unreachable("bad Op value");
        break;
    }

    const bool IsNop = Op == RISCV::ANDI ? Combined == -1 : Combined == 0;
    if (IsNop) {
        if (Rd == RsReg.get())
            return Match{};

        Rule::Match Result;
        Result.Replacement.emplace_back(
            MCInstBuilder(RISCV::ADDI).addReg(Rd).addReg(RsReg.get()).addImm(0));
        return Result;
    }

    Rule::Match Result;
    Result.Replacement.emplace_back(
        MCInstBuilder(Op).addReg(Rd).addReg(RsReg.get()).addImm(Combined));
    return Result;
}

} // namespace loonglint::RISCV
