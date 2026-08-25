// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/Rules/ShiftDoubleRule.hpp"

#include "loonglint/MCInstMatcher.hpp"

#include "MCTargetDesc/LoongArchMCTargetDesc.h"

#include "llvm/MC/MCInstBuilder.h"

#include <cassert>
#include <cstdint>

using namespace llvm;

namespace loonglint {

StringRef ShiftDoubleRule::getID() const {
    return "integer/shift-self-add";
}

StringRef ShiftDoubleRule::getDescription() const {
    return "fuse double-and-shift into one shift";
}

unsigned ShiftDoubleRule::getInstructionCount() const {
    return 2;
}

std::optional<Rule::Match> ShiftDoubleRule::match(ArrayRef<Instruction> Instructions,
                                                  const Context &) const {
    using namespace LowLevelInstMatcherDSL;

    assert(Instructions.size() == 2 && "integer/shift-self-add requires two instructions");

    const MCInst &F = Instructions[0].Inst;
    const MCInst &S = Instructions[1].Inst;

    for (const auto &[AddOp, SlliOp, IsD] : {
             std::make_tuple(LoongArch::ADD_W, LoongArch::SLLI_W, false),
             std::make_tuple(LoongArch::ADD_D, LoongArch::SLLI_D, true),
         }) {
        const int64_t MaxShift = IsD ? 63 : 31;

        // Form 1: ADD rd, rj, rj ; SLLI rd, rd, Shamt  ->  SLLI rd, rj, Shamt+1
        {
            Reg AddRdReg, AddRjReg, AddRkReg;
            if (matchInst(F, AddOp, AddRdReg, AddRjReg, AddRjReg)) {
                const MCRegister AddRd = AddRdReg.get(), AddRj = AddRjReg.get();
                Imm ShamtImm;
                if (matchInst(S, SlliOp, AddRdReg, AddRdReg, ShamtImm)) {
                    const int64_t Shamt = ShamtImm.get();
                    if (Shamt >= 1 && Shamt + 1 <= MaxShift) {
                        Rule::Match Result;
                        Result.Replacement.emplace_back(
                            MCInstBuilder(SlliOp).addReg(AddRd).addReg(AddRj).addImm(Shamt + 1));
                        return Result;
                    }
                }
            }
        }

        // Form 2: SLLI rd, rj, Shamt ; ADD rd, rd, rd  ->  SLLI rd, rj, Shamt+1
        {
            Reg SlliRdReg, SlliRjReg;
            Imm ShamtImm;
            if (matchInst(F, SlliOp, SlliRdReg, SlliRjReg, ShamtImm)) {
                const MCRegister SlliRd = SlliRdReg.get(), SlliRj = SlliRjReg.get();
                const int64_t Shamt = ShamtImm.get();
                if (matchInst(S, AddOp, SlliRdReg, SlliRdReg, SlliRdReg) && Shamt >= 1 &&
                    Shamt + 1 <= MaxShift) {
                    Rule::Match Result;
                    Result.Replacement.emplace_back(
                        MCInstBuilder(SlliOp).addReg(SlliRd).addReg(SlliRj).addImm(Shamt + 1));
                    return Result;
                }
            }
        }
    }

    return std::nullopt;
}

} // namespace loonglint
