// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/Rules/RISCV/ShiftMaskRule.hpp"

#include "loonglint/MCInstMatcher.hpp"

#include "MCTargetDesc/RISCVMCTargetDesc.h"

#include "llvm/MC/MCInstBuilder.h"
#include "llvm/Support/MathExtras.h"

#include <cassert>
#include <cstdint>

using namespace llvm;

namespace loonglint::RISCV {

namespace RISCV = ::llvm::RISCV;

ShiftMaskRule::ShiftMaskRule(const RISCVSpec &RISCVAS) : RISCVAS(RISCVAS) {}

StringRef ShiftMaskRule::getID() const {
    return "riscv:integer/shift-mask";
}

StringRef ShiftMaskRule::getDescription() const {
    return "fuse shift pair into andi mask";
}

unsigned ShiftMaskRule::getInstructionCount() const {
    return 2;
}

std::optional<Rule::Match> ShiftMaskRule::match(ArrayRef<Instruction> Instructions,
                                                const Context &) const {
    using namespace LowLevelInstMatcherDSL;

    assert(Instructions.size() == 2 && "integer/shift-mask requires two instructions");

    const MCInst &F = Instructions[0].Inst;
    const MCInst &S = Instructions[1].Inst;
    const int64_t XLEN = RISCVAS.isRV64() ? 64 : 32;

    // slli Rd, Rs, N; srli Rd, Rd, N -> andi Rd, Rs, (1 << (XLEN - N)) - 1.
    do {
        Reg RdReg, RsReg;
        Imm NImm;
        if (!matchInst(F, RISCV::SLLI, RdReg, RsReg, NImm) ||
            !matchInst(S, RISCV::SRLI, RdReg, RdReg, NImm))
            break;
        const int64_t N = NImm.get();

        if (N < 1 || RdReg.get() == RISCV::X0)
            break;

        const int64_t Mask = static_cast<int64_t>((uint64_t(1) << (XLEN - N)) - 1);
        if (!isInt<12>(Mask))
            break;

        Rule::Match Result;
        Result.Replacement.emplace_back(
            MCInstBuilder(RISCV::ANDI).addReg(RdReg.get()).addReg(RsReg.get()).addImm(Mask));
        return Result;

    } while (0);

    // srli Rd, Rs, N; slli Rd, Rd, N -> andi Rd, Rs, -(1 << N).
    do {
        Reg RdReg, RsReg;
        Imm NImm;
        if (!matchInst(F, RISCV::SRLI, RdReg, RsReg, NImm) ||
            !matchInst(S, RISCV::SLLI, RdReg, RdReg, NImm))
            break;
        const int64_t N = NImm.get();

        if (N < 1 || RdReg.get() == RISCV::X0)
            break;

        const int64_t Mask = -(int64_t(1) << N);
        if (!isInt<12>(Mask))
            break;

        Rule::Match Result;
        Result.Replacement.emplace_back(
            MCInstBuilder(RISCV::ANDI).addReg(RdReg.get()).addReg(RsReg.get()).addImm(Mask));
        return Result;
    } while (0);

    return std::nullopt;
}

} // namespace loonglint::RISCV
