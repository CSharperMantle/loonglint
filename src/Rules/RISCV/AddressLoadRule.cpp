// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/Rules/RISCV/AddressLoadRule.hpp"

#include "loonglint/MCInstMatcher.hpp"

#include "MCTargetDesc/RISCVMCTargetDesc.h"

#include "llvm/MC/MCInstBuilder.h"
#include "llvm/Support/MathExtras.h"

#include <cassert>
#include <cstdint>

using namespace llvm;

namespace loonglint::RISCV {

namespace RISCV = ::llvm::RISCV;

AddressLoadRule::AddressLoadRule(const RISCVSpec &RISCVAS) : RISCVAS(RISCVAS) {}

StringRef AddressLoadRule::getID() const {
    return "riscv:memory/address-load";
}

StringRef AddressLoadRule::getDescription() const {
    return "fold 'addi'-to-address into integer load offset";
}

unsigned AddressLoadRule::getInstructionCount() const {
    return 2;
}

std::optional<Rule::Match> AddressLoadRule::match(ArrayRef<Instruction> Instructions,
                                                  const Context &) const {
    using namespace LowLevelInstMatcherDSL;

    assert(Instructions.size() == 2 && "memory/address-load requires two instructions");

    const MCInst &F = Instructions[0].Inst;
    const MCInst &S = Instructions[1].Inst;

    Reg AddiRdReg, AddiRs1Reg;
    Imm AddiImm;
    if (!matchInst(F, RISCV::ADDI, AddiRdReg, AddiRs1Reg, AddiImm))
        return std::nullopt;
    const MCRegister AddiRd = AddiRdReg.get();
    const int64_t AddiOffset = AddiImm.get();

    if (AddiRd == RISCV::X0)
        return std::nullopt;

    Rule::Match Result;
    const auto TryLoad = [&](unsigned LoadOp) {
        Imm LoadImm;
        // load Rd, Rd, Imm1: the load overwrites the address temporary.
        if (!matchInst(S, LoadOp, AddiRdReg, AddiRdReg, LoadImm))
            return false;

        const int64_t Combined = AddiOffset + LoadImm.get();
        if (!isInt<12>(Combined))
            return false;

        Result.Replacement.emplace_back(
            MCInstBuilder(LoadOp).addReg(AddiRd).addReg(AddiRs1Reg.get()).addImm(Combined));
        return true;
    };

    for (const unsigned LoadOp : {RISCV::LB, RISCV::LH, RISCV::LW, RISCV::LBU, RISCV::LHU})
        if (TryLoad(LoadOp))
            return Result;
    if (RISCVAS.isRV64())
        for (const unsigned LoadOp : {RISCV::LWU, RISCV::LD})
            if (TryLoad(LoadOp))
                return Result;

    return std::nullopt;
}

} // namespace loonglint::RISCV
