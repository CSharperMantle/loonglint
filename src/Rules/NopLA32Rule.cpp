// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/Rules/NopLA32Rule.hpp"

#include "loonglint/MCInstMatcher.hpp"

#include "MCTargetDesc/LoongArchMCTargetDesc.h"

#include <cassert>

using namespace llvm;

namespace loonglint {

StringRef NopLA32Rule::getID() const {
    return "integer/nop-la32";
}

StringRef NopLA32Rule::getDescription() const {
    return "delete or replace non-canonical LA32 NOP instruction";
}

unsigned NopLA32Rule::getInstructionCount() const {
    return 1;
}

bool NopLA32Rule::shouldRun(const Context &Ctx) const {
    return Ctx.Arch == Architecture::LoongArch32;
}

std::optional<Rule::Match> NopLA32Rule::match(ArrayRef<Instruction> Instructions,
                                              const Context &) const {
    using namespace LowLevelInstMatcherDSL;

    assert(Instructions.size() == 1 && "nop-la32 rule requires one instruction");

    const MCInst &I = Instructions.front().Inst;
    const unsigned Opcode = I.getOpcode();

    switch (Opcode) {
    case LoongArch::ADD_W:
    case LoongArch::SUB_W: {
        // ADD.W/SUB.W rd, rd, $zero (LA32: no sign extension of the word result).
        Reg RdReg;
        if (matchInst(I, Opcode, RdReg, RdReg, Reg(LoongArch::R0)))
            return Match{};
        return std::nullopt;
    }
    case LoongArch::ADDI_W:
    case LoongArch::SLLI_W:
    case LoongArch::SRLI_W:
    case LoongArch::SRAI_W:
    case LoongArch::ROTRI_W: {
        // ADDI.W/SLLI.W/SRLI.W/SRAI.W/ROTRI.W rd, rd, 0
        Reg RdReg;
        if (matchInst(I, Opcode, RdReg, RdReg, Imm(0)))
            return Match{};
        return std::nullopt;
    }
    default:
        return std::nullopt;
    }

    return std::nullopt;
}

} // namespace loonglint
