// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/RISCV/RISCVInstr.hpp"

#include "loonglint/MCInstMatcher.hpp"

#include "MCTargetDesc/RISCVMCTargetDesc.h"

using namespace llvm;

namespace loonglint::RISCV {

namespace RISCV = ::llvm::RISCV;

// https://docs.riscv.org/reference/isa/v20260120/unpriv/rv32.html#t-rv32i-hints
// https://github.com/riscv/riscv-isa-manual/blob/v20260120/src/rv32.adoc#user-content-t-rv32i-hints
static std::optional<HintKind> classifyHintRV32I(const MCInst &Inst) {
    using namespace LowLevelInstMatcherDSL;

    switch (Inst.getOpcode()) {
    case RISCV::LUI:
    case RISCV::AUIPC:
        if (!matchInst(Inst, Inst.getOpcode(), Reg(RISCV::X0)))
            return std::nullopt;
        return HintKind::FutureStandard;
    case RISCV::ADDI: {
        Reg Rs1Reg;
        Imm TheImm;
        if (!matchInst(Inst, RISCV::ADDI, Reg(RISCV::X0), Rs1Reg, TheImm))
            return std::nullopt;
        if ((Rs1Reg.get() != RISCV::X0) || (TheImm.get() != 0))
            return HintKind::FutureStandard;
        return std::nullopt;
    }
    case RISCV::ANDI:
    case RISCV::ORI:
    case RISCV::XORI:
        if (!matchInst(Inst, Inst.getOpcode(), Reg(RISCV::X0)))
            return std::nullopt;
        return HintKind::FutureStandard;
    case RISCV::ADD: {
        Reg Rs1Reg, Rs2Reg;
        if (!matchInst(Inst, RISCV::ADD, Reg(RISCV::X0), Rs1Reg, Rs2Reg))
            return std::nullopt;
        if (Rs1Reg.get() != RISCV::X0)
            return HintKind::FutureStandard;
        if (Rs2Reg.get() < RISCV::X2 || Rs2Reg.get() > RISCV::X5)
            return HintKind::FutureStandard;
        return HintKind::Standard;
    }
    case RISCV::SLLI: {
        Reg Rs1Reg;
        Imm ShamtImm;
        if (!matchInst(Inst, RISCV::SLLI, Reg(RISCV::X0), Rs1Reg, ShamtImm))
            return std::nullopt;
        if (Rs1Reg.get() == RISCV::X0 && ShamtImm.get() == 31)
            return HintKind::Standard;
        return HintKind::Custom;
    }
    case RISCV::SRAI: {
        Reg Rs1Reg;
        Imm ShamtImm;
        if (!matchInst(Inst, RISCV::SRAI, Reg(RISCV::X0), Rs1Reg, ShamtImm))
            return std::nullopt;
        if (Rs1Reg.get() == RISCV::X0 && ShamtImm.get() == 7)
            return HintKind::Standard;
        return HintKind::Custom;
    }
    case RISCV::SUB:
    case RISCV::AND:
    case RISCV::OR:
    case RISCV::XOR:
    case RISCV::SLL:
    case RISCV::SRL:
    case RISCV::SRA:
        if (!matchInst(Inst, Inst.getOpcode(), Reg(RISCV::X0)))
            return std::nullopt;
        return HintKind::FutureStandard;
    case RISCV::FENCE: {
        Imm PredImm, SuccImm;
        if (!matchInst(Inst, RISCV::FENCE, PredImm, SuccImm))
            return std::nullopt;
        if (PredImm.get() == 0b0000 && SuccImm.get() != 0b0000)
            return HintKind::FutureStandard;
        if (PredImm.get() != 0b0001 && SuccImm.get() == 0b0000)
            return HintKind::FutureStandard;
        if (PredImm.get() == 0b0001 && SuccImm.get() == 0b0000)
            return HintKind::Standard;
        return std::nullopt;
    }
    case RISCV::SLTI:
    case RISCV::SLTIU:
    case RISCV::SRLI:
    case RISCV::SLT:
    case RISCV::SLTU:
        if (!matchInst(Inst, Inst.getOpcode(), Reg(RISCV::X0)))
            return std::nullopt;
        return HintKind::Custom;
    default:
        return std::nullopt;
    }
}

// https://docs.riscv.org/reference/isa/v20260120/unpriv/rv64.html#rv64i-h
// https://github.com/riscv/riscv-isa-manual/blob/v20260120/src/rv64.adoc#user-content-rv64i-h
static std::optional<HintKind> classifyHintRV64I(const MCInst &Inst) {
    using namespace LowLevelInstMatcherDSL;

    switch (Inst.getOpcode()) {
    case RISCV::ADDIW:
    case RISCV::ADDW:
    case RISCV::SUBW:
    case RISCV::SLLW:
    case RISCV::SRLW:
    case RISCV::SRAW:
        if (!matchInst(Inst, Inst.getOpcode(), Reg(RISCV::X0)))
            return std::nullopt;
        return HintKind::FutureStandard;
    case RISCV::SLLIW:
    case RISCV::SRLIW:
    case RISCV::SRAIW:
        if (!matchInst(Inst, Inst.getOpcode(), Reg(RISCV::X0)))
            return std::nullopt;
        return HintKind::Custom;
    default:
        return std::nullopt;
    }
}

// https://docs.riscv.org/reference/isa/v20260120/unpriv/c-st-ext.html#rvc-t-hints
// https://github.com/riscv/riscv-isa-manual/blob/v20260120/src/c-st-ext.adoc#user-content-rvc-t-hints
static std::optional<HintKind> classifyHintRVC(const MCInst &Inst) {
    using namespace LowLevelInstMatcherDSL;

    switch (Inst.getOpcode()) {
    case RISCV::C_NOP:
        return HintKind::Standard;
    case RISCV::C_NOP_HINT:
        return HintKind::FutureStandard;
    case RISCV::C_ADDI: {
        Reg RdReg;
        if (!matchInst(Inst, RISCV::C_ADDI, RdReg, RdReg, Imm(0)))
            return std::nullopt;
        if (RdReg.get() != RISCV::X0)
            return HintKind::FutureStandard;
        return std::nullopt;
    }
    case RISCV::C_LI:
        if (!matchInst(Inst, RISCV::C_LI, Reg(RISCV::X0)))
            return std::nullopt;
        return HintKind::FutureStandard;
    case RISCV::C_LUI: {
        Imm TheImm;
        if (!matchInst(Inst, RISCV::C_LUI, Reg(RISCV::X0), TheImm))
            return std::nullopt;
        if (TheImm.get() != 0)
            return HintKind::FutureStandard;
        return std::nullopt;
    }
    case RISCV::C_MV: {
        Reg Rs2Reg;
        if (!matchInst(Inst, RISCV::C_MV, Reg(RISCV::X0), Rs2Reg))
            return std::nullopt;
        if (Rs2Reg.get() != RISCV::X0)
            return HintKind::FutureStandard;
        return std::nullopt;
    }
    case RISCV::C_ADD: {
        Reg Rs2Reg;
        if (!matchInst(Inst, RISCV::C_ADD, Reg(RISCV::X0), Reg(RISCV::X0), Rs2Reg))
            return std::nullopt;
        if (Rs2Reg.get() >= RISCV::X2 && Rs2Reg.get() <= RISCV::X5)
            return HintKind::Standard;
        if (Rs2Reg.get() != RISCV::X0)
            return HintKind::FutureStandard;
        return std::nullopt;
    }
    case RISCV::C_SLLI: {
        Reg RdReg;
        Imm ShamtImm;
        if (!matchInst(Inst, RISCV::C_SLLI, RdReg, RdReg, ShamtImm))
            return std::nullopt;
        if (RdReg.get() == RISCV::X0 || ShamtImm.get() == 0)
            return HintKind::Custom;
        return std::nullopt;
    }
    case RISCV::C_SRLI:
    case RISCV::C_SRAI:
        if (matchInst(Inst, Inst.getOpcode(), Reg(), Reg(), Imm(0)))
            return HintKind::Custom;
        return std::nullopt;
    default:
        return std::nullopt;
    }
}

std::optional<HintKind> classifyHint(const llvm::MCInst &Inst) {
    if (std::optional<HintKind> Kind = classifyHintRV32I(Inst))
        return Kind;
    if (std::optional<HintKind> Kind = classifyHintRV64I(Inst))
        return Kind;
    if (std::optional<HintKind> Kind = classifyHintRVC(Inst))
        return Kind;
    return std::nullopt;
}

std::optional<MCRegister> matchZeroWrite(const MCInst &Inst) {
    using namespace LowLevelInstMatcherDSL;

    Reg RdReg, RsReg;
    switch (Inst.getOpcode()) {
    case RISCV::ADDI:
        if (!matchInst(Inst, RISCV::ADDI, RdReg, Reg(RISCV::X0), Imm(0)))
            return std::nullopt;
        break;
    case RISCV::C_LI:
        if (!matchInst(Inst, RISCV::C_LI, RdReg, Imm(0)))
            return std::nullopt;
        break;
    case RISCV::XOR:
    case RISCV::SUB:
    case RISCV::SUBW:
    case RISCV::SLT:
    case RISCV::SLTU:
        if (!matchInst(Inst, Inst.getOpcode(), RdReg, RsReg, RsReg))
            return std::nullopt;
        break;
    case RISCV::AND: {
        Reg Rs1Reg, Rs2Reg;
        if (!matchInst(Inst, Inst.getOpcode(), RdReg, Rs1Reg, Rs2Reg) ||
            (Rs1Reg.get() != RISCV::X0 && Rs2Reg.get() != RISCV::X0))
            return std::nullopt;
        break;
    }
    case RISCV::ANDI: {
        Imm TheImm;
        if (!matchInst(Inst, Inst.getOpcode(), RdReg, RsReg, TheImm) ||
            (RsReg.get() != RISCV::X0 && TheImm.get() != 0))
            return std::nullopt;
        break;
    }
    case RISCV::SLTIU:
        if (!matchInst(Inst, Inst.getOpcode(), RdReg, Reg(), Imm(0)))
            return std::nullopt;
        break;
    case RISCV::SLLI:
    case RISCV::SRLI:
    case RISCV::SRAI:
        if (!matchInst(Inst, Inst.getOpcode(), RdReg, Reg(RISCV::X0), Imm()))
            return std::nullopt;
        break;
    case RISCV::SLL:
    case RISCV::SRL:
    case RISCV::SRA:
        if (!matchInst(Inst, Inst.getOpcode(), RdReg, Reg(RISCV::X0), Reg()))
            return std::nullopt;
        break;
    case RISCV::SLLIW:
    case RISCV::SRLIW:
    case RISCV::SRAIW:
        if (!matchInst(Inst, Inst.getOpcode(), RdReg, Reg(RISCV::X0), Imm()))
            return std::nullopt;
        break;
    case RISCV::SLLW:
    case RISCV::SRLW:
    case RISCV::SRAW:
        if (!matchInst(Inst, Inst.getOpcode(), RdReg, Reg(RISCV::X0), Reg()))
            return std::nullopt;
        break;
    case RISCV::LUI:
        if (!matchInst(Inst, Inst.getOpcode(), RdReg, Imm(0)))
            return std::nullopt;
        break;
    case RISCV::ADD:
    case RISCV::OR:
    case RISCV::ADDW:
        if (!matchInst(Inst, Inst.getOpcode(), RdReg, Reg(RISCV::X0), Reg(RISCV::X0)))
            return std::nullopt;
        break;
    case RISCV::C_XOR:
        if (!matchInst(Inst, RISCV::C_XOR, RdReg, RdReg, RdReg))
            return std::nullopt;
        break;
    default:
        return std::nullopt;
    }
    return RdReg.get();
}

} // namespace loonglint::RISCV
