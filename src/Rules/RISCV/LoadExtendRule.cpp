// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/Rules/RISCV/LoadExtendRule.hpp"

#include "loonglint/MCInstMatcher.hpp"

#include "MCTargetDesc/RISCVMCTargetDesc.h"

#include <cassert>

using namespace llvm;

namespace loonglint::RISCV {

namespace RISCV = ::llvm::RISCV;

StringRef LoadExtendRule::getID() const {
    return "riscv:memory/load-extend";
}

StringRef LoadExtendRule::getDescription() const {
    return "delete redundant load zero extension";
}

unsigned LoadExtendRule::getInstructionCount() const {
    return 2;
}

std::optional<Rule::Match> LoadExtendRule::match(ArrayRef<Instruction> Instructions,
                                                 const Context &) const {
    using namespace LowLevelInstMatcherDSL;

    assert(Instructions.size() == 2 && "memory/load-extend requires two instructions");

    const MCInst &F = Instructions[0].Inst;
    const MCInst &S = Instructions[1].Inst;

    // lbu Rd, Rs1, Off; andi Rd, Rd, 255 -> delete the andi. LBU already
    // zero-extends the byte; the sign-extending loads belong to SextWRule and
    // the Zbb/Zba extension rules.
    Reg LdRdReg;
    if (!matchInst(F, RISCV::LBU, LdRdReg, Reg(), Imm()) ||
        !matchInst(S, RISCV::ANDI, LdRdReg, LdRdReg, Imm(255)) || LdRdReg.get() == RISCV::X0)
        return std::nullopt;

    Rule::Match Result;
    Result.Replacement.emplace_back(F);
    return Result;
}

} // namespace loonglint::RISCV
