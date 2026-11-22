// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/LoongArch/LoongArchInstr.hpp"

#include "loonglint/MCInstMatcher.hpp"

#include "MCTargetDesc/LoongArchMCTargetDesc.h"

using namespace llvm;

namespace loonglint::LoongArch {

namespace LoongArch = ::llvm::LoongArch;

std::optional<MCRegister> matchZeroWrite(const MCInst &Inst) {
    using namespace LowLevelInstMatcherDSL;

    Reg RdReg, RjReg, RkReg;
    Imm TheImm, MsbImm, LsbImm;
    switch (Inst.getOpcode()) {
    case LoongArch::ADD_W:
    case LoongArch::ADD_D:
        if (!matchInst(Inst, Inst.getOpcode(), RdReg, Reg(LoongArch::R0), Reg(LoongArch::R0)))
            return std::nullopt;
        break;
    case LoongArch::SUB_W:
    case LoongArch::SUB_D:
        if (!matchInst(Inst, Inst.getOpcode(), RdReg, RjReg, RkReg) || RjReg.get() != RkReg.get())
            return std::nullopt;
        break;
    case LoongArch::ADDI_W:
    case LoongArch::ADDI_D:
    case LoongArch::ADDU16I_D:
        if (!matchInst(Inst, Inst.getOpcode(), RdReg, Reg(LoongArch::R0), Imm(0)))
            return std::nullopt;
        break;
    case LoongArch::ALSL_W:
    case LoongArch::ALSL_WU:
    case LoongArch::ALSL_D:
        if (!matchInst(Inst, Inst.getOpcode(), RdReg, Reg(LoongArch::R0), Reg(LoongArch::R0),
                       Imm()))
            return std::nullopt;
        break;
    case LoongArch::LU12I_W:
        if (!matchInst(Inst, Inst.getOpcode(), RdReg, Imm(0)))
            return std::nullopt;
        break;
    case LoongArch::LU52I_D:
        if (!matchInst(Inst, Inst.getOpcode(), RdReg, Reg(LoongArch::R0), Imm(0)))
            return std::nullopt;
        break;
    case LoongArch::SLT:
    case LoongArch::SLTU:
        if (!matchInst(Inst, Inst.getOpcode(), RdReg, RjReg, RkReg) || RjReg.get() != RkReg.get())
            return std::nullopt;
        break;
    case LoongArch::SLTI:
        if (!matchInst(Inst, Inst.getOpcode(), RdReg, Reg(LoongArch::R0), TheImm) ||
            TheImm.get() > 0)
            return std::nullopt;
        break;
    case LoongArch::SLTUI:
        if (!matchInst(Inst, Inst.getOpcode(), RdReg, Reg(), Imm(0)))
            return std::nullopt;
        break;
    case LoongArch::AND: {
        if (!matchInst(Inst, Inst.getOpcode(), RdReg, RjReg, RkReg) ||
            (RjReg.get() != LoongArch::R0 && RkReg.get() != LoongArch::R0))
            return std::nullopt;
        break;
    }
    case LoongArch::OR:
        if (!matchInst(Inst, Inst.getOpcode(), RdReg, Reg(LoongArch::R0), Reg(LoongArch::R0)))
            return std::nullopt;
        break;
    case LoongArch::XOR:
        if (!matchInst(Inst, Inst.getOpcode(), RdReg, RjReg, RkReg) || RjReg.get() != RkReg.get())
            return std::nullopt;
        break;
    case LoongArch::ANDN:
        if (!matchInst(Inst, Inst.getOpcode(), RdReg, Reg(LoongArch::R0), Reg()))
            return std::nullopt;
        break;
    case LoongArch::ANDI: {
        if (!matchInst(Inst, Inst.getOpcode(), RdReg, RjReg, TheImm) ||
            (RjReg.get() != LoongArch::R0 && TheImm.get() != 0))
            return std::nullopt;
        break;
    }
    case LoongArch::ORI:
    case LoongArch::XORI:
        if (!matchInst(Inst, Inst.getOpcode(), RdReg, Reg(LoongArch::R0), Imm(0)))
            return std::nullopt;
        break;
    case LoongArch::MUL_W:
    case LoongArch::MULH_W:
    case LoongArch::MULH_WU:
    case LoongArch::MUL_D:
    case LoongArch::MULH_D:
    case LoongArch::MULH_DU:
    case LoongArch::MULW_D_W:
    case LoongArch::MULW_D_WU: {
        if (!matchInst(Inst, Inst.getOpcode(), RdReg, RjReg, RkReg) ||
            (RjReg.get() != LoongArch::R0 && RkReg.get() != LoongArch::R0))
            return std::nullopt;
        break;
    }
    case LoongArch::SLL_W:
    case LoongArch::SRL_W:
    case LoongArch::SRA_W:
    case LoongArch::ROTR_W:
    case LoongArch::SLL_D:
    case LoongArch::SRL_D:
    case LoongArch::SRA_D:
    case LoongArch::ROTR_D:
        if (!matchInst(Inst, Inst.getOpcode(), RdReg, Reg(LoongArch::R0), Reg()))
            return std::nullopt;
        break;
    case LoongArch::SLLI_W:
    case LoongArch::SRLI_W:
    case LoongArch::SRAI_W:
    case LoongArch::ROTRI_W:
    case LoongArch::SLLI_D:
    case LoongArch::SRLI_D:
    case LoongArch::SRAI_D:
    case LoongArch::ROTRI_D:
        if (!matchInst(Inst, Inst.getOpcode(), RdReg, Reg(LoongArch::R0), Imm()))
            return std::nullopt;
        break;
    case LoongArch::EXT_W_B:
    case LoongArch::EXT_W_H:
    case LoongArch::CLO_W:
    case LoongArch::CTO_W:
    case LoongArch::CLO_D:
    case LoongArch::CTO_D:
    case LoongArch::REVB_2H:
    case LoongArch::REVB_4H:
    case LoongArch::REVB_2W:
    case LoongArch::REVB_D:
    case LoongArch::REVH_2W:
    case LoongArch::REVH_D:
    case LoongArch::BITREV_4B:
    case LoongArch::BITREV_8B:
    case LoongArch::BITREV_W:
    case LoongArch::BITREV_D:
        if (!matchInst(Inst, Inst.getOpcode(), RdReg, Reg(LoongArch::R0)))
            return std::nullopt;
        break;
    case LoongArch::BYTEPICK_W:
    case LoongArch::BYTEPICK_D:
        if (!matchInst(Inst, Inst.getOpcode(), RdReg, RjReg, RkReg, TheImm) ||
            RkReg.get() != LoongArch::R0 || (TheImm.get() != 0 && RjReg.get() != LoongArch::R0))
            return std::nullopt;
        break;
    case LoongArch::BSTRINS_W:
        if (!matchInst(Inst, LoongArch::BSTRINS_W, RdReg, Reg(), Reg(LoongArch::R0), Imm(31),
                       Imm(0)))
            return std::nullopt;
        break;
    case LoongArch::BSTRINS_D:
        if (!matchInst(Inst, LoongArch::BSTRINS_D, RdReg, Reg(), Reg(LoongArch::R0), Imm(63),
                       Imm(0)))
            return std::nullopt;
        break;
    case LoongArch::BSTRPICK_W:
    case LoongArch::BSTRPICK_D:
        if (!matchInst(Inst, Inst.getOpcode(), RdReg, Reg(LoongArch::R0), MsbImm, LsbImm) ||
            MsbImm.get() < LsbImm.get())
            return std::nullopt;
        break;
    case LoongArch::MASKEQZ: {
        if (!matchInst(Inst, Inst.getOpcode(), RdReg, RjReg, RkReg) ||
            (RjReg.get() != LoongArch::R0 && RkReg.get() != LoongArch::R0))
            return std::nullopt;
        break;
    }
    case LoongArch::MASKNEZ:
        if (!matchInst(Inst, Inst.getOpcode(), RdReg, Reg(LoongArch::R0), Reg()))
            return std::nullopt;
        break;
    default:
        return std::nullopt;
    }
    return RdReg.get();
}

} // namespace loonglint::LoongArch
