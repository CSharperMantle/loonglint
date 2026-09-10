// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/Rules/RISCV/PCBranchRule.hpp"

#include "loonglint/MCInstMatcher.hpp"

#include "MCTargetDesc/RISCVMCTargetDesc.h"

#include "llvm/MC/MCInstBuilder.h"
#include "llvm/Support/MathExtras.h"

#include <cassert>
#include <cstdint>

using namespace llvm;

namespace loonglint::RISCV {

namespace RISCV = ::llvm::RISCV;

StringRef PCBranchRule::getID() const {
    return "riscv:control/pc-branch";
}

StringRef PCBranchRule::getDescription() const {
    return "fuse 'auipc; jalr' call into 'jal'";
}

unsigned PCBranchRule::getInstructionCount() const {
    return 2;
}

std::optional<Rule::Match> PCBranchRule::match(ArrayRef<Instruction> Instructions,
                                               const Context &) const {
    using namespace LowLevelInstMatcherDSL;

    assert(Instructions.size() == 2 && "control/pc-branch requires two instructions");

    const MCInst &F = Instructions[0].Inst;
    const MCInst &S = Instructions[1].Inst;

    // auipc T, Hi20; jalr T, T, Lo12 -> jal T, Target. The call form is
    // provable from the shared destination; the tail form (jalr X0, T, Lo12)
    // needs Tmp to be dead and stays deferred.
    Reg TReg;
    Imm HiImm;
    if (!matchInst(F, RISCV::AUIPC, TReg, HiImm))
        return std::nullopt;
    const MCRegister T = TReg.get();

    if (T == RISCV::X0)
        return std::nullopt;

    Reg JalrRdReg, JalrRs1Reg;
    Imm LoImm;
    if (!matchInst(S, RISCV::JALR, JalrRdReg, JalrRs1Reg, LoImm))
        return std::nullopt;
    if (JalrRdReg.get() != T || JalrRs1Reg.get() != T)
        return std::nullopt;

    const uint64_t PC = Instructions[0].Address;
    const int64_t Hi = SignExtend64<32>(static_cast<uint64_t>(HiImm.get()) << 12);
    const int64_t Lo = LoImm.get();
    // JALR clears bit 0 of the computed address.
    const uint64_t Target = (PC + static_cast<uint64_t>(Hi + Lo)) & ~uint64_t(1);
    const int64_t Offset = static_cast<int64_t>(Target - PC);
    if (!isInt<21>(Offset))
        return std::nullopt;

    Rule::Match Result;
    Result.Replacement.emplace_back(MCInstBuilder(RISCV::JAL).addReg(T).addImm(Offset));
    return Result;
}

} // namespace loonglint::RISCV
