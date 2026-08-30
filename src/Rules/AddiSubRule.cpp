// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/Rules/AddiSubRule.hpp"

#include "loonglint/MCInstMatcher.hpp"

#include "MCTargetDesc/LoongArchMCTargetDesc.h"

#include "llvm/MC/MCInstBuilder.h"
#include "llvm/Support/MathExtras.h"

#include <cassert>
#include <cstdint>

using namespace llvm;

namespace loonglint {

StringRef AddiSubRule::getID() const {
    return "integer/addi-sub";
}

StringRef AddiSubRule::getDescription() const {
    return "fold common-base ADDI+SUB into a constant";
}

unsigned AddiSubRule::getInstructionCount() const {
    return 2;
}

std::optional<Rule::Match> AddiSubRule::match(ArrayRef<Instruction> Instructions,
                                              const Context &Ctx) const {
    using namespace LowLevelInstMatcherDSL;

    assert(Instructions.size() == 2 && "integer/addi-sub requires two instructions");

    const MCInst &F = Instructions[0].Inst;
    const MCInst &S = Instructions[1].Inst;

    // Width rows: (ADDI opcode, SUB opcode, replacement ADDI opcode, LA64-only).
    // The mixed ADDI.D + SUB.W row is sound because SUB.W consumes only the
    // low 32 bits, so the fold holds modulo 2^32.
    for (const auto &[AddOp, SubOp, ReplacementOp, Needs64] : {
             std::make_tuple(LoongArch::ADDI_W, LoongArch::SUB_W, LoongArch::ADDI_W, false),
             std::make_tuple(LoongArch::ADDI_D, LoongArch::SUB_D, LoongArch::ADDI_D, true),
             std::make_tuple(LoongArch::ADDI_D, LoongArch::SUB_W, LoongArch::ADDI_W, true),
         }) {
        if (Needs64 && Ctx.Arch != Architecture::LoongArch64)
            continue;

        Reg RdReg, RjReg;
        Imm Si12Imm;
        if (!matchInst(F, AddOp, RdReg, RjReg, Si12Imm))
            continue;
        const MCRegister Rd = RdReg.get();
        const MCRegister Rj = RjReg.get();
        const int64_t Si12 = Si12Imm.get();

        // R0 discards the ADDI result, so the SUB would read 0; an aliased Rj
        // would make the SUB read the overwritten base instead of the base.
        if (Rd == LoongArch::R0 || Rd == Rj)
            continue;

        // SUB Rd, Rd, Rj : (Rj + Si12) - Rj = Si12.
        if (matchInst(S, SubOp, RdReg, RdReg, RjReg)) {
            Rule::Match Result;
            Result.Replacement.emplace_back(
                MCInstBuilder(ReplacementOp).addReg(Rd).addReg(LoongArch::R0).addImm(Si12));
            return Result;
        }

        // SUB Rd, Rj, Rd : Rj - (Rj + Si12) = -Si12.
        if (matchInst(S, SubOp, RdReg, RjReg, RdReg)) {
            Rule::Match Result;
            if (isInt<12>(-Si12)) {
                Result.Replacement.emplace_back(
                    MCInstBuilder(ReplacementOp).addReg(Rd).addReg(LoongArch::R0).addImm(-Si12));
            } else {
                // -Si12 = 2048 overflows the signed 12-bit range; ORI
                // zero-extends the same value.
                Result.Replacement.emplace_back(MCInstBuilder(LoongArch::ORI)
                                                    .addReg(Rd)
                                                    .addReg(LoongArch::R0)
                                                    .addImm(static_cast<unsigned>(-Si12)));
            }
            return Result;
        }
    }

    return std::nullopt;
}

} // namespace loonglint
