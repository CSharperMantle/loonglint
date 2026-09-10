// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/Rules/RISCV/SextWElimRule.hpp"

#include "loonglint/MCInstMatcher.hpp"

#include "MCTargetDesc/RISCVBaseInfo.h"
#include "MCTargetDesc/RISCVMCTargetDesc.h"

#include <cassert>

using namespace llvm;

namespace loonglint::RISCV {

namespace RISCV = ::llvm::RISCV;

SextWElimRule::SextWElimRule(const RISCVSpec &RISCVAS) : RISCVAS(RISCVAS) {}

StringRef SextWElimRule::getID() const {
    return "riscv:integer/sext-w-elim";
}

StringRef SextWElimRule::getDescription() const {
    return "delete redundant 'sext.w' after sign-extending word producer";
}

unsigned SextWElimRule::getInstructionCount() const {
    return 2;
}

bool SextWElimRule::shouldRun(const Context &) const {
    return RISCVAS.isRV64();
}

std::optional<Rule::Match> SextWElimRule::match(ArrayRef<Instruction> Instructions,
                                                const Context &Ctx) const {
    using namespace LowLevelInstMatcherDSL;

    assert(Instructions.size() == 2 && "integer/sext-w-elim requires two instructions");

    const MCInst &F = Instructions[0].Inst;
    const MCInst &S = Instructions[1].Inst;

    // addiw Rd, Rd, 0 (printed sext.w, also encoded as C_ADDIW) after a
    // producer whose word result is already sign-extended: delete the explicit
    // extension. The producer set is exactly LLVM's IsSignExtendingOpW flag;
    // C_ADDIW carries no flag but is the same instruction.
    Reg RdReg;
    if (!matchInst(S, RISCV::ADDIW, RdReg, RdReg, Imm(0)) &&
        !matchInst(S, RISCV::C_ADDIW, RdReg, RdReg, Imm(0)))
        return std::nullopt;
    const MCRegister Rd = RdReg.get();

    if (Rd == RISCV::X0)
        return std::nullopt;

    const unsigned ProducerOp = F.getOpcode();
    const bool IsSext = ProducerOp == RISCV::C_ADDIW ||
                        (Ctx.MII.get(ProducerOp).TSFlags & RISCVII::IsSignExtendingOpWMask) != 0;
    if (!IsSext || !matchInst(F, ProducerOp, RdReg))
        return std::nullopt;

    return Match{};
}

} // namespace loonglint::RISCV
