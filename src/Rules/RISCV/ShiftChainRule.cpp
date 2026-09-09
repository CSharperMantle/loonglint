// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/Rules/RISCV/ShiftChainRule.hpp"

#include "loonglint/MCInstMatcher.hpp"

#include "MCTargetDesc/RISCVMCTargetDesc.h"

#include "llvm/MC/MCInstBuilder.h"

#include <algorithm>
#include <cassert>
#include <cstdint>

using namespace llvm;

namespace loonglint::RISCV {

namespace RISCV = ::llvm::RISCV;

namespace {

// First-instruction forms keyed by opcode. |BaseOp| is the uncompressed
// replacement opcode, |CompressedOp| the compressed continuation accepting the
// same direction, and |Arithmetic| selects the saturating right-shift fold.
struct ShiftForm {
    unsigned Opcode;
    unsigned BaseOp;
    unsigned CompressedOp;
    bool Compressed;
    bool Arithmetic;
};

constexpr ShiftForm Forms[] = {
    {RISCV::SLLI, RISCV::SLLI, RISCV::C_SLLI, false, false},
    {RISCV::C_SLLI, RISCV::SLLI, RISCV::C_SLLI, true, false},
    {RISCV::SRLI, RISCV::SRLI, RISCV::C_SRLI, false, false},
    {RISCV::C_SRLI, RISCV::SRLI, RISCV::C_SRLI, true, false},
    {RISCV::SRAI, RISCV::SRAI, RISCV::C_SRAI, false, true},
    {RISCV::C_SRAI, RISCV::SRAI, RISCV::C_SRAI, true, true},
};

} // namespace

ShiftChainRule::ShiftChainRule(const RISCVSpec &RISCVAS) : RISCVAS(RISCVAS) {}

StringRef ShiftChainRule::getID() const {
    return "riscv:integer/shift-chain";
}

StringRef ShiftChainRule::getDescription() const {
    return "fuse adjacent same-direction shifts";
}

unsigned ShiftChainRule::getInstructionCount() const {
    return 2;
}

std::optional<Rule::Match> ShiftChainRule::match(ArrayRef<Instruction> Instructions,
                                                 const Context &) const {
    using namespace LowLevelInstMatcherDSL;

    assert(Instructions.size() == 2 && "integer/shift-chain requires two instructions");

    const MCInst &F = Instructions[0].Inst;
    const MCInst &S = Instructions[1].Inst;
    const int64_t XLEN = RISCVAS.isRV64() ? 64 : 32;

    const ShiftForm *Form = nullptr;
    for (const auto &Candidate : Forms)
        if (Candidate.Opcode == F.getOpcode())
            Form = &Candidate;
    if (!Form)
        return std::nullopt;

    Reg RdReg, RsReg;
    Imm FirstImm;
    const bool FirstMatched = Form->Compressed ? matchInst(F, Form->Opcode, RdReg, RdReg, FirstImm)
                                               : matchInst(F, Form->Opcode, RdReg, RsReg, FirstImm);
    if (!FirstMatched)
        return std::nullopt;
    const MCRegister Rd = RdReg.get();
    // The compressed forms read and write the same register.
    const MCRegister Rs = Form->Compressed ? Rd : RsReg.get();
    const int64_t First = FirstImm.get();

    if (Rd == RISCV::X0 || First < 1)
        return std::nullopt;

    // The second shift must keep the direction and overwrite Rd.
    Imm SecondImm;
    if (!matchInst(S, Form->BaseOp, RdReg, RdReg, SecondImm) &&
        !matchInst(S, Form->CompressedOp, RdReg, RdReg, SecondImm))
        return std::nullopt;
    const int64_t Second = SecondImm.get();

    if (Second < 1)
        return std::nullopt;

    // Logical shifts whose sum reaches XLEN zero the result, which no single
    // shift can express; arithmetic shifts clamp to the sign-fill shift.
    int64_t Combined = First + Second;
    if (Form->Arithmetic)
        Combined = std::min(Combined, XLEN - 1);
    else if (Combined > XLEN - 1)
        return std::nullopt;

    Rule::Match Result;
    Result.Replacement.emplace_back(
        MCInstBuilder(Form->BaseOp).addReg(Rd).addReg(Rs).addImm(Combined));
    return Result;
}

} // namespace loonglint::RISCV
