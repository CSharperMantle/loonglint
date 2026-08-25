// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/Rules/ZeroExtendRule.hpp"

#include "loonglint/MCInstMatcher.hpp"

#include "MCTargetDesc/LoongArchMCTargetDesc.h"

#include "llvm/MC/MCInstBuilder.h"

#include <cassert>
#include <cstdint>

using namespace llvm;

namespace loonglint {

StringRef ZeroExtendRule::getID() const {
    return "integer/zero-extend";
}

StringRef ZeroExtendRule::getDescription() const {
    return "fold shift pair into zero-extending BSTRPICK.[DW]";
}

unsigned ZeroExtendRule::getInstructionCount() const {
    return 2;
}

std::optional<Rule::Match> ZeroExtendRule::match(ArrayRef<Instruction> Instructions,
                                                 const Context &) const {
    using namespace LowLevelInstMatcherDSL;

    assert(Instructions.size() == 2 && "integer/zero-extend requires two instructions");

    const MCInst &F = Instructions[0].Inst;
    const MCInst &S = Instructions[1].Inst;
    const unsigned Op = F.getOpcode();

    bool IsD = false;
    if (Op == LoongArch::SLLI_D)
        IsD = true;
    else if (Op == LoongArch::SLLI_W)
        IsD = false;
    else
        return std::nullopt;

    Reg SlliRdReg, SlliRjReg;
    Imm ShamtImm;
    if (!matchInst(F, Op, SlliRdReg, SlliRjReg, ShamtImm))
        return std::nullopt;
    const MCRegister SlliRd = SlliRdReg.get();
    const MCRegister SlliRj = SlliRjReg.get();
    const int64_t Shamt = ShamtImm.get();

    // Second shift: SRLI.{W,D} SlliRd, SlliRd, Shamt (same amount).
    const unsigned SrOp = IsD ? LoongArch::SRLI_D : LoongArch::SRLI_W;
    if (!matchInst(S, SrOp, SlliRdReg, SlliRdReg, ShamtImm))
        return std::nullopt;

    const int64_t Width = IsD ? 64 : 32;
    if (Shamt < 1 || Shamt >= Width)
        return std::nullopt;

    const int64_t Msb = Width - Shamt - 1;
    const unsigned PickOp = IsD ? LoongArch::BSTRPICK_D : LoongArch::BSTRPICK_W;
    Rule::Match Result;
    Result.Replacement.emplace_back(
        MCInstBuilder(PickOp).addReg(SlliRd).addReg(SlliRj).addImm(Msb).addImm(0));
    return Result;
}

} // namespace loonglint
