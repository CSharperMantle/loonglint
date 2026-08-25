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
    return "fold address ADDI.[DW] into LD.[DW] offset";
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

    for (const auto &[AddiOp, LdOp, IsD] : {
             std::make_tuple(LoongArch::ADDI_D, LoongArch::LD_D, true),
             std::make_tuple(LoongArch::ADDI_W, LoongArch::LD_W, false),
         }) {
        if (IsD && Ctx.Arch != Architecture::LoongArch64)
            continue;
        if (!IsD && Ctx.Arch != Architecture::LoongArch32)
            continue;

        Reg AddiRdReg, AddiRjReg;
        Imm AddiSi12Imm;
        if (!matchInst(F, AddiOp, AddiRdReg, AddiRjReg, AddiSi12Imm))
            continue;
        const MCRegister AddiRd = AddiRdReg.get(), AddiRj = AddiRjReg.get();
        const int64_t AddiSi12 = AddiSi12Imm.get();

        // LD rd, rd, Si12: base is the address temporary, overwritten by the load.
        Imm LdSi12Imm;
        if (!matchInst(S, LdOp, AddiRdReg, AddiRdReg, LdSi12Imm))
            continue;
        const int64_t LdSi12 = LdSi12Imm.get();

        const int64_t Combined = AddiSi12 + LdSi12;
        if (!isInt<12>(Combined))
            continue;

        Rule::Match Result;
        Result.Replacement.emplace_back(
            MCInstBuilder(LdOp).addReg(AddiRd).addReg(AddiRj).addImm(Combined));
        return Result;
    }

    return std::nullopt;
}

} // namespace loonglint
