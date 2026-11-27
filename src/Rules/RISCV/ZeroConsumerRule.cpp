// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/Rules/RISCV/ZeroConsumerRule.hpp"

#include "loonglint/RISCV/RISCVInstr.hpp"

#include "MCTargetDesc/RISCVMCTargetDesc.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"

#include <cassert>

using namespace llvm;

namespace loonglint::RISCV {

namespace RISCV = ::llvm::RISCV;

// Reject if zero might have special meanings.
static constexpr unsigned OpcodeBlacklist[] = {
    // https://docs.riscv.org/reference/isa/v20260120/unpriv/v-st-ext.html#30-1-6-2-avl-encoding
    RISCV::VSETVLI,
    RISCV::VSETIVLI,
    RISCV::VSETVL,
    // https://docs.riscv.org/reference/isa/v20260120/unpriv/v-st-ext.html#30-1-7-5-vector-constant-stride-instructions
    RISCV::VLSE8_V,
    RISCV::VLSE16_V,
    RISCV::VLSE32_V,
    RISCV::VLSE64_V,
    RISCV::VSSE8_V,
    RISCV::VSSE16_V,
    RISCV::VSSE32_V,
    RISCV::VSSE64_V,
};

StringRef ZeroConsumerRule::getID() const {
    return "riscv:integer/zero-consumer";
}

StringRef ZeroConsumerRule::getDescription() const {
    return "replace consumers of the materialized 0 with register 'zero'";
}

unsigned ZeroConsumerRule::getInstructionCount() const {
    return 2;
}

std::optional<Rule::Match> ZeroConsumerRule::match(ArrayRef<Instruction> Instructions,
                                                   const Context &Ctx) const {
    assert(Instructions.size() == 2 && "integer/zero-consumer requires two instructions");

    const MCInst &F = Instructions[0].Inst;
    const MCInst &S = Instructions[1].Inst;

    if (classifyHint(F))
        return std::nullopt;

    const std::optional<MCRegister> ZeroRd = matchZeroWrite(F);
    if (!ZeroRd || *ZeroRd == RISCV::X0)
        return std::nullopt;
    const MCRegister Rd = *ZeroRd;

    if (is_contained(OpcodeBlacklist, S.getOpcode()))
        return std::nullopt;

    const MCInstrDesc &MID = Ctx.MII.get(S.getOpcode());

    MCInst Swapped = S;
    bool Changed = false;
    // For each operand...
    for (const auto [Idx, MOI] : enumerate(MID.operands())) {
        // ...that is a use, ...
        if (Idx < MID.getNumDefs())
            continue;
        MCOperand &Op = Swapped.getOperand(Idx);
        // ...if it equals Rd...
        if (!Op.isReg() || Op.getReg() != Rd)
            continue;
        // ...and is not tied to any defs...
        const int Tied = MID.getOperandConstraint(Idx, MCOI::TIED_TO);
        if (Tied >= 0 && static_cast<unsigned>(Tied) < MID.getNumDefs())
            continue;
        assert(MOI.RegClass >= 0);
        // ...and its slot can house the zero register...
        if (!Ctx.MRI.getRegClass(MOI.RegClass).contains(RISCV::X0))
            continue;
        // ...then plug it in.
        Op.setReg(RISCV::X0);
        Changed = true;
    }
    if (!Changed)
        return std::nullopt;

    Rule::Match Result;
    Result.Replacement.emplace_back(F);
    Result.Replacement.emplace_back(Swapped);
    return Result;
}

} // namespace loonglint::RISCV
