// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/Rules/NopLA64Rule.hpp"

#include "loonglint/MCInstMatcher.hpp"

#include "MCTargetDesc/LoongArchMCTargetDesc.h"

#include <cassert>

using namespace llvm;

namespace loonglint {

StringRef NopLA64Rule::getID() const {
    return "integer/nop-la64";
}

StringRef NopLA64Rule::getDescription() const {
    return "delete or replace non-canonical LA64 NOP instruction";
}

unsigned NopLA64Rule::getInstructionCount() const {
    return 1;
}

bool NopLA64Rule::shouldRun(const Context &Ctx) const {
    return Ctx.Arch == Architecture::LoongArch64;
}

std::optional<Rule::Match> NopLA64Rule::match(ArrayRef<Instruction> Instructions,
                                              const Context &) const {
    using namespace LowLevelInstMatcherDSL;

    assert(Instructions.size() == 1 && "nop-la64 rule requires one instruction");

    const MCInst &I = Instructions.front().Inst;
    const unsigned Opcode = I.getOpcode();

    switch (Opcode) {
    case LoongArch::ADD_D:
    case LoongArch::SUB_D: {
        // ADD.D/SUB.D rd, rd, $zero (.D does not sign-extend on LA64).
        Reg RdReg;
        if (matchInst(I, Opcode, RdReg, RdReg, Reg(LoongArch::R0)))
            return Match{};
        return std::nullopt;
    }
    case LoongArch::ADDI_D:
    case LoongArch::SLLI_D:
    case LoongArch::SRLI_D:
    case LoongArch::SRAI_D:
    case LoongArch::ROTRI_D: {
        // ADDI.D/SLLI.D/SRLI.D/SRAI.D/ROTRI.D rd, rd, 0
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
