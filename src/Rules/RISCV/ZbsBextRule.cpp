// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/Rules/RISCV/ZbsBextRule.hpp"

#include "loonglint/MCInstMatcher.hpp"

#include "MCTargetDesc/RISCVMCTargetDesc.h"

#include "llvm/MC/MCInstBuilder.h"

#include <cassert>
#include <cstdint>

using namespace llvm;

namespace loonglint::RISCV {

namespace RISCV = ::llvm::RISCV;

ZbsBextRule::ZbsBextRule(const RISCVSpec &RISCVAS) : RISCVAS(RISCVAS) {}

StringRef ZbsBextRule::getID() const {
    return "riscv:integer/zbs-bext";
}

StringRef ZbsBextRule::getDescription() const {
    return "fuse 'srl/srli; andi 1' into Zbs 'bext/bexti'";
}

unsigned ZbsBextRule::getInstructionCount() const {
    return 2;
}

bool ZbsBextRule::shouldRun(const Context &) const {
    return RISCVAS.hasZbs();
}

std::optional<Rule::Match> ZbsBextRule::match(ArrayRef<Instruction> Instructions,
                                              const Context &) const {
    using namespace LowLevelInstMatcherDSL;

    assert(Instructions.size() == 2 && "integer/zbs-bext requires two instructions");

    const MCInst &F = Instructions[0].Inst;
    const MCInst &S = Instructions[1].Inst;

    // srli Tmp, Rs, Imm; andi Tmp, Tmp, 1 -> bexti Tmp, Rs, Imm. A zero shift
    // is only a NopRule NOP when it rewrites its own source; otherwise it is a
    // copy the extraction absorbs.
    do {
        Reg TmpReg, RsReg;
        Imm ShamtImm;
        if (!matchInst(F, RISCV::SRLI, TmpReg, RsReg, ShamtImm))
            break;
        const int64_t Shamt = ShamtImm.get();

        const bool ShiftIsNop = Shamt == 0 && TmpReg.get() == RsReg.get();

        if (ShiftIsNop || TmpReg.get() == RISCV::X0 ||
            !matchInst(S, RISCV::ANDI, TmpReg, TmpReg, Imm(1)))
            break;

        Rule::Match Result;
        Result.Replacement.emplace_back(
            MCInstBuilder(RISCV::BEXTI).addReg(TmpReg.get()).addReg(RsReg.get()).addImm(Shamt));
        return Result;
    } while (0);

    // srl Tmp, Rs1, Rs2; andi Tmp, Tmp, 1 -> bext Tmp, Rs1, Rs2.
    do {
        Reg TmpReg, Rs1Reg, Rs2Reg;
        if (!matchInst(F, RISCV::SRL, TmpReg, Rs1Reg, Rs2Reg) || TmpReg.get() == RISCV::X0 ||
            !matchInst(S, RISCV::ANDI, TmpReg, TmpReg, Imm(1)))
            break;

        Rule::Match Result;
        Result.Replacement.emplace_back(MCInstBuilder(RISCV::BEXT)
                                            .addReg(TmpReg.get())
                                            .addReg(Rs1Reg.get())
                                            .addReg(Rs2Reg.get()));
        return Result;
    } while (0);

    return std::nullopt;
}

} // namespace loonglint::RISCV
