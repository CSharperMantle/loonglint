// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/Rules/AddressLoadRule.hpp"

#include "loonglint/MCInstMatcher.hpp"

#include "MCTargetDesc/LoongArchMCTargetDesc.h"

#include "llvm/MC/MCInstBuilder.h"
#include "llvm/Support/MathExtras.h"

#include <cassert>
#include <cstdint>

using namespace llvm;

namespace loonglint {

StringRef AddressLoadRule::getID() const {
    return "memory/address-load";
}

StringRef AddressLoadRule::getDescription() const {
    return "fold address ADDI.[DW] into integer load offset";
}

unsigned AddressLoadRule::getInstructionCount() const {
    return 2;
}

std::optional<Rule::Match> AddressLoadRule::match(ArrayRef<Instruction> Instructions,
                                                  const Context &Ctx) const {
    using namespace LowLevelInstMatcherDSL;

    assert(Instructions.size() == 2 && "memory/address-load requires two instructions");

    const MCInst &F = Instructions[0].Inst;
    const MCInst &S = Instructions[1].Inst;

    const unsigned AddiOp =
        Ctx.Arch == Architecture::LoongArch64 ? LoongArch::ADDI_D : LoongArch::ADDI_W;

    Reg AddiRdReg, AddiRjReg;
    Imm AddiSi12Imm;
    if (!matchInst(F, AddiOp, AddiRdReg, AddiRjReg, AddiSi12Imm))
        return std::nullopt;
    const MCRegister AddiRd = AddiRdReg.get();
    const MCRegister AddiRj = AddiRjReg.get();
    const int64_t AddiSi12 = AddiSi12Imm.get();

    if (AddiRd == LoongArch::R0)
        return std::nullopt;

    Rule::Match Result;
    const auto TryLoad = [&](unsigned LoadOp, bool IsScaled) {
        Imm LoadOffsetImm;
        if (!matchInst(S, LoadOp, AddiRdReg, AddiRdReg, LoadOffsetImm))
            return false;

        const int64_t Combined = AddiSi12 + LoadOffsetImm.get();
        // LDPTR's MCOperand immediate is already shifted properly by the encoder/decoder.
        if (IsScaled ? !isShiftedInt<14, 2>(Combined) : !isInt<12>(Combined))
            return false;

        Result.Replacement.emplace_back(
            MCInstBuilder(LoadOp).addReg(AddiRd).addReg(AddiRj).addImm(Combined));
        return true;
    };

    if (Ctx.Arch == Architecture::LoongArch32) {
        for (const unsigned LoadOp : {LoongArch::LD_B, LoongArch::LD_H, LoongArch::LD_W,
                                      LoongArch::LD_BU, LoongArch::LD_HU})
            if (TryLoad(LoadOp, false))
                return Result;
    } else {
        for (const unsigned LoadOp :
             {LoongArch::LD_B, LoongArch::LD_H, LoongArch::LD_W, LoongArch::LD_D, LoongArch::LD_BU,
              LoongArch::LD_HU, LoongArch::LD_WU})
            if (TryLoad(LoadOp, false))
                return Result;
        for (const unsigned LoadOp : {LoongArch::LDPTR_W, LoongArch::LDPTR_D})
            if (TryLoad(LoadOp, true))
                return Result;
    }

    return std::nullopt;
}

} // namespace loonglint
