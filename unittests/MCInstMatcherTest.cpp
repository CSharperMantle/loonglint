// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/MCInstMatcher.hpp"

#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCRegister.h"
#include "gtest/gtest.h"

#include <initializer_list>

using namespace llvm;
using namespace loonglint::LowLevelInstMatcherDSL;

static MCInst makeInst(unsigned Opcode, std::initializer_list<MCOperand> Operands) {
    MCInst Result;
    Result.setOpcode(Opcode);
    for (const MCOperand &Operand : Operands)
        Result.addOperand(Operand);
    return Result;
}

namespace {

TEST(MCInstMatcherTest, CapturesTypedOperands) {
    const MCInst Inst =
        makeInst(7, {MCOperand::createReg(MCRegister(2)), MCOperand::createImm(42)});

    Reg Register;
    Imm Immediate;
    EXPECT_TRUE(matchInst(Inst, 7, Register, Immediate));
    EXPECT_EQ(Register.get(), MCRegister(2));
    EXPECT_EQ(Immediate.get(), 42);
}

TEST(MCInstMatcherTest, RejectsWrongOperandKinds) {
    const MCInst Inst =
        makeInst(7, {MCOperand::createReg(MCRegister(2)), MCOperand::createImm(42)});

    EXPECT_FALSE(matchInst(Inst, 7, Imm(), Reg()));
}

TEST(MCInstMatcherTest, RequiresRepeatedCaptureEquality) {
    const MCInst Equal =
        makeInst(7, {MCOperand::createReg(MCRegister(2)), MCOperand::createReg(MCRegister(2))});
    const MCInst Different =
        makeInst(7, {MCOperand::createReg(MCRegister(2)), MCOperand::createReg(MCRegister(3))});

    Reg Register;
    EXPECT_TRUE(matchInst(Equal, 7, Register, Register));
    EXPECT_FALSE(matchInst(Different, 7, Register, Register));
    EXPECT_EQ(Register.get(), MCRegister(2));
}

TEST(MCInstMatcherTest, RequiresExactOperandCount) {
    const MCInst Inst =
        makeInst(7, {MCOperand::createReg(MCRegister(2)), MCOperand::createImm(42)});

    EXPECT_FALSE(matchInst(Inst, 7, Reg()));
    EXPECT_FALSE(matchInst(Inst, 7, Reg(), Imm(), Skip()));
    EXPECT_FALSE(matchInst(Inst, 8, Reg(), Imm()));
}

TEST(MCInstMatcherTest, SkipConsumesAnyOperand) {
    const MCInst Inst =
        makeInst(7, {MCOperand::createReg(MCRegister(2)), MCOperand::createImm(42)});

    Reg Register;
    EXPECT_TRUE(matchInst(Inst, 7, Register, Skip()));
    EXPECT_EQ(Register.get(), MCRegister(2));
}

TEST(MCInstMatcherTest, FailedMatchRollsBackCaptures) {
    const MCInst Failure =
        makeInst(7, {MCOperand::createReg(MCRegister(2)), MCOperand::createImm(41)});
    const MCInst Success =
        makeInst(7, {MCOperand::createReg(MCRegister(3)), MCOperand::createImm(42)});

    Reg Register;
    EXPECT_FALSE(matchInst(Failure, 7, Register, Imm(42)));
    EXPECT_TRUE(matchInst(Success, 7, Register, Imm(42)));
    EXPECT_EQ(Register.get(), MCRegister(3));
}

TEST(MCInstMatcherTest, FreshCapturesSupportIndependentAlternatives) {
    const MCInst Inst =
        makeInst(7, {MCOperand::createReg(MCRegister(3)), MCOperand::createImm(42)});

    Reg FirstRegister;
    EXPECT_FALSE(matchInst(Inst, 7, FirstRegister, Imm(41)));

    Reg SecondRegister;
    EXPECT_TRUE(matchInst(Inst, 7, SecondRegister, Imm(42)));
    EXPECT_EQ(SecondRegister.get(), MCRegister(3));
}

} // namespace
