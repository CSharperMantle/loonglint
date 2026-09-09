// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/Rules/RISCV/SextWRule.hpp"

#include "loonglint/MCInstMatcher.hpp"

#include "MCTargetDesc/RISCVBaseInfo.h"
#include "MCTargetDesc/RISCVMCTargetDesc.h"

#include "llvm/MC/MCInstBuilder.h"

#include <cassert>

using namespace llvm;

namespace loonglint::RISCV {

namespace RISCV = ::llvm::RISCV;

SextWRule::SextWRule(const RISCVSpec &RISCVAS) : RISCVAS(RISCVAS) {}

StringRef SextWRule::getID() const {
    return "riscv:integer/sext-w";
}

StringRef SextWRule::getDescription() const {
    return "delete redundant sext.w";
}

unsigned SextWRule::getInstructionCount() const {
    return 2;
}

bool SextWRule::shouldRun(const Context &) const {
    return RISCVAS.isRV64();
}

std::optional<Rule::Match> SextWRule::match(ArrayRef<Instruction> Instructions,
                                            const Context &Ctx) const {
    using namespace LowLevelInstMatcherDSL;

    assert(Instructions.size() == 2 && "integer/sext-w requires two instructions");

    const MCInst &F = Instructions[0].Inst;
    const MCInst &S = Instructions[1].Inst;

    // addiw Rd, Rd, 0 (printed sext.w, also encoded as C_ADDIW) after a
    // producer whose word result is already sign-extended: delete the explicit
    // extension. The producer set is exactly LLVM's IsSignExtendingOpW flag;
    // C_ADDIW carries no flag but is the same instruction.
    do {
        Reg RdReg;
        if (!matchInst(S, RISCV::ADDIW, RdReg, RdReg, Imm(0)) &&
            !matchInst(S, RISCV::C_ADDIW, RdReg, RdReg, Imm(0)))
            break;
        const MCRegister Rd = RdReg.get();

        if (Rd == RISCV::X0)
            break;

        const unsigned ProducerOp = F.getOpcode();
        const bool IsSext = ProducerOp == RISCV::C_ADDIW || (Ctx.MII.get(ProducerOp).TSFlags &
                                                             RISCVII::IsSignExtendingOpWMask) != 0;
        if (!IsSext || !matchInst(F, ProducerOp, RdReg))
            break;

        return Match{};
    } while (0);

    // slli Rd, Rs, 32; srai Rd, Rd, 32 -> addiw Rd, Rs, 0.
    do {
        Reg RdReg, RsReg;
        Imm ShamtImm;
        if (!matchInst(F, RISCV::SLLI, RdReg, RsReg, ShamtImm) || ShamtImm.get() != 32)
            break;
        if (!matchInst(S, RISCV::SRAI, RdReg, RdReg, Imm(32)))
            break;

        const MCRegister Rd = RdReg.get();
        if (Rd == RISCV::X0)
            break;

        Rule::Match Result;
        Result.Replacement.emplace_back(
            MCInstBuilder(RISCV::ADDIW).addReg(Rd).addReg(RsReg.get()).addImm(0));
        return Result;
    } while (0);

    return std::nullopt;
}

} // namespace loonglint::RISCV
