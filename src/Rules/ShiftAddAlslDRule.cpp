// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/Rules/ShiftAddAlslDRule.hpp"

#include "loonglint/MCInstMatcher.hpp"

#include "MCTargetDesc/LoongArchMCTargetDesc.h"

#include "llvm/MC/MCInstBuilder.h"

#include <cassert>

using namespace llvm;

namespace loonglint {

StringRef ShiftAddAlslDRule::getID() const {
    return "integer/shift-add-alsl-d";
}

StringRef ShiftAddAlslDRule::getDescription() const {
    return "fuse shift and add into ALSL.D";
}

unsigned ShiftAddAlslDRule::getInstructionCount() const {
    return 2;
}

bool ShiftAddAlslDRule::shouldRun(const Context &Ctx) const {
    return Ctx.Arch == Architecture::LoongArch64;
}

std::optional<Rule::Match> ShiftAddAlslDRule::match(ArrayRef<Instruction> Instructions,
                                                    const Context &) const {
    using namespace LowLevelInstMatcherDSL;

    assert(Instructions.size() == 2 && "integer/shift-add-alsl-d requires two instructions");

    Reg SlliRdReg, SlliRjReg;
    Imm ShamtImm;
    if (!matchInst(Instructions[0].Inst, LoongArch::SLLI_D, SlliRdReg, SlliRjReg, ShamtImm))
        return std::nullopt;
    const MCRegister SlliRd = SlliRdReg.get();
    const MCRegister SlliRj = SlliRjReg.get();
    const int64_t Shamt = ShamtImm.get();

    if (Shamt < 1 || Shamt > 4 || SlliRdReg.get() == LoongArch::R0)
        return std::nullopt;

    Reg AddRjReg, AddRkReg;
    if (!matchInst(Instructions[1].Inst, LoongArch::ADD_D, SlliRdReg, AddRjReg, AddRkReg))
        return std::nullopt;
    const MCRegister AddRj = AddRjReg.get();
    const MCRegister AddRk = AddRkReg.get();

    std::optional<MCRegister> AlslRk;
    if (AddRj == SlliRd && AddRk != SlliRd) {
        AlslRk = AddRk;
    } else if (AddRj != SlliRd && AddRk == SlliRd) {
        AlslRk = AddRj;
    } else {
        return std::nullopt;
    }

    Rule::Match Result;
    Result.Replacement.emplace_back(MCInstBuilder(LoongArch::ALSL_D)
                                        .addReg(SlliRd)
                                        .addReg(SlliRj)
                                        .addReg(*AlslRk)
                                        .addImm(Shamt));
    return Result;
}

} // namespace loonglint
