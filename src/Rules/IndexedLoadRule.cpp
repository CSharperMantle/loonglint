// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/Rules/IndexedLoadRule.hpp"

#include "loonglint/MCInstMatcher.hpp"

#include "MCTargetDesc/LoongArchMCTargetDesc.h"

#include "llvm/MC/MCInstBuilder.h"

#include <cassert>

using namespace llvm;

namespace loonglint {

StringRef IndexedLoadRule::getID() const {
    return "memory/indexed-load";
}

StringRef IndexedLoadRule::getDescription() const {
    return "fold ADD.D and zero-offset LD.[BHWD] into LDX.[BHWD]";
}

unsigned IndexedLoadRule::getInstructionCount() const {
    return 2;
}

bool IndexedLoadRule::shouldRun(const Context &Ctx) const {
    return Ctx.Arch == Architecture::LoongArch64;
}

std::optional<Rule::Match> IndexedLoadRule::match(ArrayRef<Instruction> Instructions,
                                                  const Context &) const {
    using namespace LowLevelInstMatcherDSL;

    assert(Instructions.size() == 2 && "memory/indexed-load requires two instructions");

    const MCInst &F = Instructions[0].Inst;
    const MCInst &S = Instructions[1].Inst;

    Reg AddRdReg, AddRjReg, AddRkReg;
    if (!matchInst(F, LoongArch::ADD_D, AddRdReg, AddRjReg, AddRkReg))
        return std::nullopt;
    const MCRegister AddRd = AddRdReg.get();
    const MCRegister AddRj = AddRjReg.get();
    const MCRegister AddRk = AddRkReg.get();

    // Address sources must not alias the temporary; the indexed load reads them
    // after the ADD has written it.
    if (AddRj == AddRd || AddRk == AddRd)
        return std::nullopt;

    for (const auto &[LdOp, LdxOp] : {
             std::make_tuple(LoongArch::LD_B, LoongArch::LDX_B),
             std::make_tuple(LoongArch::LD_H, LoongArch::LDX_H),
             std::make_tuple(LoongArch::LD_W, LoongArch::LDX_W),
             std::make_tuple(LoongArch::LD_D, LoongArch::LDX_D),
         }) {
        // LD rd, rd, 0 : zero offset, base is the address temporary.
        if (matchInst(S, LdOp, AddRdReg, AddRdReg, Imm(0))) {
            Rule::Match Result;
            Result.Replacement.emplace_back(
                MCInstBuilder(LdxOp).addReg(AddRd).addReg(AddRj).addReg(AddRk));
            return Result;
        }
    }

    return std::nullopt;
}

} // namespace loonglint
