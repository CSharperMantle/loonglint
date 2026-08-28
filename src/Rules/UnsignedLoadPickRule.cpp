// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/Rules/UnsignedLoadPickRule.hpp"

#include "loonglint/MCInstMatcher.hpp"

#include "MCTargetDesc/LoongArchMCTargetDesc.h"

#include <cassert>
#include <utility>

using namespace llvm;

namespace loonglint {

StringRef UnsignedLoadPickRule::getID() const {
    return "memory/unsigned-load-pick";
}

StringRef UnsignedLoadPickRule::getDescription() const {
    return "delete redundant zero-extension pick after unsigned load";
}

unsigned UnsignedLoadPickRule::getInstructionCount() const {
    return 2;
}

std::optional<Rule::Match> UnsignedLoadPickRule::match(ArrayRef<Instruction> Instructions,
                                                       const Context &Ctx) const {
    using namespace LowLevelInstMatcherDSL;

    assert(Instructions.size() == 2 && "memory/unsigned-load-pick requires two instructions");

    const MCInst &F = Instructions[0].Inst;
    const MCInst &S = Instructions[1].Inst;

    if (Ctx.Arch == Architecture::LoongArch64) {
        // BSTRPICK.D rd, rd, Msb, 0 after LD/LDX.{BU,HU,WU} re-extracts exactly
        // what the unsigned load already produced.
        for (const auto &[LdOp, Msb] : {
                 std::make_pair(LoongArch::LD_BU, 7),
                 std::make_pair(LoongArch::LD_HU, 15),
                 std::make_pair(LoongArch::LD_WU, 31),
                 std::make_pair(LoongArch::LDX_BU, 7),
                 std::make_pair(LoongArch::LDX_HU, 15),
                 std::make_pair(LoongArch::LDX_WU, 31),
             }) {
            Reg LdRdReg;
            if (matchInst(F, LdOp, LdRdReg, Reg(), Skip()) &&
                matchInst(S, LoongArch::BSTRPICK_D, LdRdReg, LdRdReg, Imm(Msb), Imm(0))) {
                Rule::Match Result;
                Result.Replacement.push_back(F);
                return Result;
            }
        }
    } else {
        // On LA32, BSTRPICK.W is a plain zero-extension of the field,
        // so the byte and halfword picks after unsigned loads are redundant.
        for (const auto &[LdOp, Msb] : {
                 std::make_pair(LoongArch::LD_BU, 7),
                 std::make_pair(LoongArch::LD_HU, 15),
             }) {
            Reg LdRdReg;
            if (matchInst(F, LdOp, LdRdReg, Reg(), Imm()) &&
                matchInst(S, LoongArch::BSTRPICK_W, LdRdReg, LdRdReg, Imm(Msb), Imm(0))) {
                Rule::Match Result;
                Result.Replacement.push_back(F);
                return Result;
            }
        }
    }

    return std::nullopt;
}

} // namespace loonglint
