// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/Rules/LoongArch/ZeroConsumerRule.hpp"

#include "loonglint/LoongArch/LoongArchInstr.hpp"

#include "MCTargetDesc/LoongArchMCTargetDesc.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"

#include <cassert>

using namespace llvm;

namespace loonglint::LoongArch {

namespace LoongArch = ::llvm::LoongArch;

StringRef ZeroConsumerRule::getID() const {
    return "loongarch:integer/zero-consumer";
}

StringRef ZeroConsumerRule::getDescription() const {
    return "replace consumers of the materialized 0 with register '$zero'";
}

unsigned ZeroConsumerRule::getInstructionCount() const {
    return 2;
}

std::optional<Rule::Match> ZeroConsumerRule::match(ArrayRef<Instruction> Instructions,
                                                   const Context &Ctx) const {
    assert(Instructions.size() == 2 && "integer/zero-consumer requires two instructions");

    const MCInst &F = Instructions[0].Inst;
    const MCInst &S = Instructions[1].Inst;

    const std::optional<MCRegister> ZeroRd = matchZeroWrite(F);
    if (!ZeroRd || *ZeroRd == LoongArch::R0)
        return std::nullopt;
    const MCRegister Rd = *ZeroRd;

    // Reject if the register number itself carries meaning. For the AM*
    // families an equal rd and rj raises an invalid-instruction exception and
    // an equal rd and rk returns an undefined result, so preserving it.
    // https://loongson.github.io/LoongArch-Documentation/LoongArch-Vol1-EN.html#atomic-memory-access-instructions
    if (is_contained(
            {
                LoongArch::AMSWAP_B,     LoongArch::AMSWAP_H,     LoongArch::AMSWAP_W,
                LoongArch::AMSWAP_D,     LoongArch::AMADD_B,      LoongArch::AMADD_H,
                LoongArch::AMADD_W,      LoongArch::AMADD_D,      LoongArch::AMAND_W,
                LoongArch::AMAND_D,      LoongArch::AMOR_W,       LoongArch::AMOR_D,
                LoongArch::AMXOR_W,      LoongArch::AMXOR_D,      LoongArch::AMMAX_W,
                LoongArch::AMMAX_D,      LoongArch::AMMIN_W,      LoongArch::AMMIN_D,
                LoongArch::AMMAX_WU,     LoongArch::AMMAX_DU,     LoongArch::AMMIN_WU,
                LoongArch::AMMIN_DU,     LoongArch::AMSWAP__DB_B, LoongArch::AMSWAP__DB_H,
                LoongArch::AMSWAP__DB_W, LoongArch::AMSWAP__DB_D, LoongArch::AMADD__DB_B,
                LoongArch::AMADD__DB_H,  LoongArch::AMADD__DB_W,  LoongArch::AMADD__DB_D,
                LoongArch::AMAND__DB_W,  LoongArch::AMAND__DB_D,  LoongArch::AMOR__DB_W,
                LoongArch::AMOR__DB_D,   LoongArch::AMXOR__DB_W,  LoongArch::AMXOR__DB_D,
                LoongArch::AMMAX__DB_W,  LoongArch::AMMAX__DB_D,  LoongArch::AMMIN__DB_W,
                LoongArch::AMMIN__DB_D,  LoongArch::AMMAX__DB_WU, LoongArch::AMMAX__DB_DU,
                LoongArch::AMMIN__DB_WU, LoongArch::AMMIN__DB_DU, LoongArch::AMCAS_B,
                LoongArch::AMCAS_H,      LoongArch::AMCAS_W,      LoongArch::AMCAS_D,
                LoongArch::AMCAS__DB_B,  LoongArch::AMCAS__DB_H,  LoongArch::AMCAS__DB_W,
                LoongArch::AMCAS__DB_D,
            },
            S.getOpcode()) &&
        S.getOperand(0).getReg() == Rd)
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
        assert(MOI.RegClass >= 0);
        // ...and its slot can house the zero register...
        if (!Ctx.MRI.getRegClass(MOI.RegClass).contains(LoongArch::R0))
            return std::nullopt;
        // ...then plug it in.
        Op.setReg(LoongArch::R0);
        Changed = true;
    }
    if (!Changed)
        return std::nullopt;

    Rule::Match Result;
    Result.Replacement.emplace_back(F);
    Result.Replacement.emplace_back(Swapped);
    return Result;
}

} // namespace loonglint::LoongArch
