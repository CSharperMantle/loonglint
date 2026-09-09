// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/Rules/RISCV/ZbbSextRule.hpp"

#include "loonglint/MCInstMatcher.hpp"

#include "MCTargetDesc/RISCVMCTargetDesc.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/MC/MCInstBuilder.h"

#include <cassert>
#include <cstdint>

using namespace llvm;

namespace loonglint::RISCV {

namespace RISCV = ::llvm::RISCV;

ZbbSextRule::ZbbSextRule(const RISCVSpec &RISCVAS) : RISCVAS(RISCVAS) {}

StringRef ZbbSextRule::getID() const {
    return "riscv:integer/zbb-sext";
}

StringRef ZbbSextRule::getDescription() const {
    return "fuse or delete sext.b/sext.h";
}

unsigned ZbbSextRule::getInstructionCount() const {
    return 2;
}

bool ZbbSextRule::shouldRun(const Context &) const {
    return RISCVAS.hasZbb();
}

std::optional<Rule::Match> ZbbSextRule::match(ArrayRef<Instruction> Instructions,
                                              const Context &) const {
    using namespace LowLevelInstMatcherDSL;

    assert(Instructions.size() == 2 && "integer/zbb-sext requires two instructions");

    const MCInst &F = Instructions[0].Inst;
    const MCInst &S = Instructions[1].Inst;
    const int64_t XLEN = RISCVAS.isRV64() ? 64 : 32;

    // slli Rd, Rs, XLEN-16/8; srai Rd, Rd, XLEN-16/8 -> sext.h/sext.b.
    do {
        Reg RdReg, RsReg;
        Imm ShamtImm;
        if (!matchInst(F, RISCV::SLLI, RdReg, RsReg, ShamtImm))
            break;
        const int64_t Shamt = ShamtImm.get();

        unsigned Op = 0;
        if (Shamt == XLEN - 16)
            Op = RISCV::SEXT_H;
        else if (Shamt == XLEN - 8)
            Op = RISCV::SEXT_B;
        else
            break;

        if (RdReg.get() == RISCV::X0 || !matchInst(S, RISCV::SRAI, RdReg, RdReg, Imm(Shamt)))
            break;

        Rule::Match Result;
        Result.Replacement.emplace_back(MCInstBuilder(Op).addReg(RdReg.get()).addReg(RsReg.get()));
        return Result;

    } while (0);

    // producer; sext.b/sext.h Rd, Rd -> delete the redundant extension.
    do {
        const unsigned SOp = S.getOpcode();
        if (!is_contained({RISCV::SEXT_B, RISCV::SEXT_H}, SOp))
            break;

        Reg RdReg;
        if (!matchInst(S, SOp, RdReg, RdReg) || RdReg.get() == RISCV::X0)
            break;
        const unsigned POp = F.getOpcode();

        bool Redundant = false;
        if (POp == RISCV::SEXT_B || POp == RISCV::LB)
            Redundant = true; // sext.b/sext.h after a byte-extended value
        else if (POp == RISCV::SEXT_H || POp == RISCV::LH)
            Redundant = SOp == RISCV::SEXT_H; // sext.b after sext.h narrows

        if (!Redundant || !matchInst(F, POp, RdReg))
            break;

        Rule::Match Result;
        Result.Replacement.emplace_back(F);
        return Result;
    } while (0);

    return std::nullopt;
}

} // namespace loonglint::RISCV
