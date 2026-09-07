// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/Rules/LoongArch/LoadZeroExtendRule.hpp"

#include "loonglint/LoongArchSpec.hpp"
#include "loonglint/MCInstMatcher.hpp"

#include "MCTargetDesc/LoongArchMCTargetDesc.h"

#include "llvm/MC/MCInstBuilder.h"

#include <cassert>
#include <cstdint>

using namespace llvm;

namespace loonglint::LoongArch {

namespace LoongArch = ::llvm::LoongArch;

LoadZeroExtendRule::LoadZeroExtendRule(const LoongArchSpec &LoongAS) : LoongAS(LoongAS) {}

StringRef LoadZeroExtendRule::getID() const {
    return "memory/load-zero-extend";
}

StringRef LoadZeroExtendRule::getDescription() const {
    return "fold load and zero-extract into unsigned load";
}

unsigned LoadZeroExtendRule::getInstructionCount() const {
    return 2;
}

std::optional<Rule::Match> LoadZeroExtendRule::match(ArrayRef<Instruction> Instructions,
                                                     const Context &Ctx) const {
    using namespace LowLevelInstMatcherDSL;

    assert(Instructions.size() == 2 && "memory/load-zero-extend requires two instructions");

    const MCInst &F = Instructions[0].Inst;
    const MCInst &S = Instructions[1].Inst;

    for (const auto &[LdOp, PickOp, UnsOp, Msb, Needs64] : {
             std::make_tuple(LoongArch::LD_B, LoongArch::BSTRPICK_D, LoongArch::LD_BU, int64_t(7),
                             true),
             std::make_tuple(LoongArch::LD_H, LoongArch::BSTRPICK_D, LoongArch::LD_HU, int64_t(15),
                             true),
             std::make_tuple(LoongArch::LD_W, LoongArch::BSTRPICK_D, LoongArch::LD_WU, int64_t(31),
                             true),
             std::make_tuple(LoongArch::LD_B, LoongArch::BSTRPICK_W, LoongArch::LD_BU, int64_t(7),
                             false),
             std::make_tuple(LoongArch::LD_H, LoongArch::BSTRPICK_W, LoongArch::LD_HU, int64_t(15),
                             false),
         }) {
        if (Needs64 && !LoongAS.is64())
            continue;
        if (!Needs64 && LoongAS.is64())
            continue;

        Reg LdRdReg, LdRjReg;
        Imm Si12Imm;
        if (!matchInst(F, LdOp, LdRdReg, LdRjReg, Si12Imm))
            continue;
        const MCRegister LdRd = LdRdReg.get();
        const MCRegister LdRj = LdRjReg.get();
        const int64_t Si12 = Si12Imm.get();

        // BSTRPICK rd, rd, Msb, 0 : extracts the loaded low field.
        if (!matchInst(S, PickOp, LdRdReg, LdRdReg, Imm(Msb), Imm(0)))
            continue;
        Rule::Match Result;
        Result.Replacement.emplace_back(
            MCInstBuilder(UnsOp).addReg(LdRd).addReg(LdRj).addImm(Si12));
        return Result;
    }

    return std::nullopt;
}

} // namespace loonglint::LoongArch
