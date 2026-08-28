// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/Rules/LoadExtendRule.hpp"

#include "loonglint/MCInstMatcher.hpp"

#include "MCTargetDesc/LoongArchMCTargetDesc.h"

#include <cassert>

using namespace llvm;

namespace loonglint {

StringRef LoadExtendRule::getID() const {
    return "memory/load-extend";
}

StringRef LoadExtendRule::getDescription() const {
    return "delete redundant load sign extension";
}

unsigned LoadExtendRule::getInstructionCount() const {
    return 2;
}

std::optional<Rule::Match> LoadExtendRule::match(ArrayRef<Instruction> Instructions,
                                                 const Context &Ctx) const {
    using namespace LowLevelInstMatcherDSL;

    assert(Instructions.size() == 2 && "memory/load-extend requires two instructions");

    const MCInst &F = Instructions[0].Inst;
    const MCInst &S = Instructions[1].Inst;

    const auto DeleteExtension = [&]() -> std::optional<Match> {
        Rule::Match Result;
        Result.Replacement.push_back(F);
        return Result;
    };

    // LD.B + EXT.W.B  ->  LD.B
    {
        Reg LdRdReg;
        if (matchInst(F, LoongArch::LD_B, LdRdReg, Reg(), Imm()) &&
            matchInst(S, LoongArch::EXT_W_B, LdRdReg, LdRdReg))
            return DeleteExtension();
    }
    // LD.H + EXT.W.H  ->  LD.H
    {
        Reg LdRdReg;
        if (matchInst(F, LoongArch::LD_H, LdRdReg, Reg(), Imm()) &&
            matchInst(S, LoongArch::EXT_W_H, LdRdReg, LdRdReg))
            return DeleteExtension();
    }
    do {
        if (Ctx.Arch == Architecture::LoongArch64) {
            // LD.W/LDPTR.W + ADDI.W/SLLI.W rd, rd, 0  ->  delete the extension. LD.W and LDPTR.W
            // both sign-extend their word result, so the word-extension idioms are
            // redundant after them.
            Reg LdRdReg;
            if (!(matchInst(F, LoongArch::LD_W, LdRdReg, Reg(), Imm()) ||
                  (matchInst(F, LoongArch::LDPTR_W, LdRdReg, Reg(), Imm()))))
                break;
            if (!(matchInst(S, LoongArch::ADDI_W, LdRdReg, LdRdReg, Imm(0)) ||
                  matchInst(S, LoongArch::SLLI_W, LdRdReg, LdRdReg, Imm(0))))
                break;
            return DeleteExtension();
        }
    } while (0);

    return std::nullopt;
}

} // namespace loonglint
