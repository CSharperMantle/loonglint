// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/Rules/RISCV/DegenerateBranchRule.hpp"

#include "loonglint/MCInstMatcher.hpp"

#include "MCTargetDesc/RISCVMCTargetDesc.h"

#include "llvm/MC/MCInstBuilder.h"

#include <cassert>
#include <cstdint>

using namespace llvm;

namespace loonglint::RISCV {

namespace RISCV = ::llvm::RISCV;

StringRef DegenerateBranchRule::getID() const {
    return "riscv:control/degenerate-branch";
}

StringRef DegenerateBranchRule::getDescription() const {
    return "simplify constant branch condition";
}

unsigned DegenerateBranchRule::getInstructionCount() const {
    return 1;
}

std::optional<Rule::Match> DegenerateBranchRule::match(ArrayRef<Instruction> Instructions,
                                                       const Context &Ctx) const {
    using namespace LowLevelInstMatcherDSL;

    assert(Instructions.size() == 1 && "control/degenerate-branch requires one instruction");

    const Instruction &DI = Instructions.front();
    const MCInst &I = DI.Inst;

    if (!Ctx.MIA.isBranch(I) || Ctx.MIA.isCall(I) || Ctx.MIA.isIndirectBranch(I))
        return std::nullopt;

    uint64_t Target = 0;
    if (!Ctx.MIA.evaluateBranch(I, DI.Address, DI.Size, Target))
        return std::nullopt;

    // A branch to the next instruction is reported by BranchToNextRule alone.
    if (Target == DI.Address + DI.Size)
        return std::nullopt;

    const unsigned Op = I.getOpcode();
    switch (Op) {
    case RISCV::BEQ:
    case RISCV::BGE:
    case RISCV::BGEU: {
        // Rs == Rs is always true -> direct jump.
        Reg RsReg;
        if (!matchInst(I, Op, RsReg, RsReg, Imm()))
            return std::nullopt;

        Rule::Match Result;
        Result.Replacement.emplace_back(MCInstBuilder(RISCV::JAL)
                                            .addReg(RISCV::X0)
                                            .addImm(static_cast<int64_t>(Target - DI.Address)));
        return Result;
    }
    case RISCV::BNE:
    case RISCV::BLT:
    case RISCV::BLTU: {
        // Rs == Rs is always false -> delete.
        Reg RsReg;
        if (!matchInst(I, Op, RsReg, RsReg, Imm()))
            return std::nullopt;
        return Match{};
    }
    default:
        return std::nullopt;
    }
}

} // namespace loonglint::RISCV
