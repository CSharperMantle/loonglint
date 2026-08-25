// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/Rules/BitExtractRule.hpp"

#include "loonglint/MCInstMatcher.hpp"

#include "MCTargetDesc/LoongArchMCTargetDesc.h"

#include "llvm/MC/MCInstBuilder.h"
#include "llvm/Support/MathExtras.h"

#include <cassert>
#include <cstdint>

using namespace llvm;

namespace loonglint {

StringRef BitExtractRule::getID() const {
    return "integer/bit-extract";
}

StringRef BitExtractRule::getDescription() const {
    return "fold shift-and-mask into BSTRPICK.[DW]";
}

unsigned BitExtractRule::getInstructionCount() const {
    return 2;
}

std::optional<Rule::Match> BitExtractRule::match(ArrayRef<Instruction> Instructions,
                                                 const Context &) const {
    using namespace LowLevelInstMatcherDSL;

    assert(Instructions.size() == 2 && "integer/bit-extract requires two instructions");

    const MCInst &F = Instructions[0].Inst;
    const MCInst &S = Instructions[1].Inst;
    const unsigned ShiftOp = F.getOpcode();

    bool IsD = false;
    switch (ShiftOp) {
    case LoongArch::SRLI_D:
    case LoongArch::SRAI_D:
        IsD = true;
        break;
    case LoongArch::SRLI_W:
    case LoongArch::SRAI_W:
        break;
    default:
        return std::nullopt;
    }

    Reg ShiftRdReg, ShiftRjReg;
    Imm LsbImm;
    if (!matchInst(F, ShiftOp, ShiftRdReg, ShiftRjReg, LsbImm))
        return std::nullopt;
    const MCRegister ShiftRd = ShiftRdReg.get(), ShiftRj = ShiftRjReg.get();
    const int64_t Lsb = LsbImm.get();
    if (Lsb < 1)
        return std::nullopt;

    // ANDI ShiftRd, ShiftRd, Mask : masks the shifted temporary.
    Imm MaskImm;
    if (!matchInst(S, LoongArch::ANDI, ShiftRdReg, ShiftRdReg, MaskImm))
        return std::nullopt;
    const uint64_t Mask = static_cast<uint64_t>(MaskImm.get());
    if (Mask == 0 || (Mask & (Mask + 1)) != 0) // must be (1 << Len) - 1
        return std::nullopt;
    const unsigned Len = static_cast<unsigned>(llvm::countr_one(Mask));
    const unsigned Width = IsD ? 64 : 32;
    if (Lsb + Len > Width)
        return std::nullopt;

    const unsigned Msb = static_cast<unsigned>(Lsb) + Len - 1;
    const unsigned PickOp = IsD ? LoongArch::BSTRPICK_D : LoongArch::BSTRPICK_W;
    Rule::Match Result;
    Result.Replacement.emplace_back(
        MCInstBuilder(PickOp).addReg(ShiftRd).addReg(ShiftRj).addImm(Msb).addImm(
            static_cast<unsigned>(Lsb)));
    return Result;
}

} // namespace loonglint
