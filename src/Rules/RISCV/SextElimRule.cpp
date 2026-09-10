// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/Rules/RISCV/SextElimRule.hpp"

#include "loonglint/MCInstMatcher.hpp"

#include "MCTargetDesc/RISCVMCTargetDesc.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/MC/MCInstBuilder.h"

#include <cassert>

using namespace llvm;

namespace loonglint::RISCV {

namespace RISCV = ::llvm::RISCV;

SextElimRule::SextElimRule(const RISCVSpec &RISCVAS) : RISCVAS(RISCVAS) {}

StringRef SextElimRule::getID() const {
    return "riscv:integer/sext-elim";
}

StringRef SextElimRule::getDescription() const {
    return "delete redundant 'sext.b/sext.h'";
}

unsigned SextElimRule::getInstructionCount() const {
    return 2;
}

bool SextElimRule::shouldRun(const Context &) const {
    return RISCVAS.hasZbb();
}

std::optional<Rule::Match> SextElimRule::match(ArrayRef<Instruction> Instructions,
                                               const Context &) const {
    using namespace LowLevelInstMatcherDSL;

    assert(Instructions.size() == 2 && "integer/sext-elim requires two instructions");

    const MCInst &F = Instructions[0].Inst;
    const MCInst &S = Instructions[1].Inst;

    // producer; sext.b/sext.h Rd, Rd -> delete the redundant extension.
    const unsigned SOp = S.getOpcode();
    if (!is_contained({RISCV::SEXT_B, RISCV::SEXT_H}, SOp))
        return std::nullopt;

    Reg RdReg;
    if (!matchInst(S, SOp, RdReg, RdReg) || RdReg.get() == RISCV::X0)
        return std::nullopt;
    const unsigned POp = F.getOpcode();

    bool Redundant = false;
    if (POp == RISCV::SEXT_B || POp == RISCV::LB)
        Redundant = true; // sext.b/sext.h after a byte-extended value
    else if (POp == RISCV::SEXT_H || POp == RISCV::LH)
        Redundant = SOp == RISCV::SEXT_H; // sext.b after sext.h narrows

    if (!Redundant || !matchInst(F, POp, RdReg))
        return std::nullopt;

    Rule::Match Result;
    Result.Replacement.emplace_back(F);
    return Result;
}

} // namespace loonglint::RISCV
