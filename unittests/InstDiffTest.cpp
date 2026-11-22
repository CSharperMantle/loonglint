// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/InstDiff.hpp"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/MC/MCInst.h"
#include "gtest/gtest.h"

#include <cstddef>
#include <initializer_list>

using namespace llvm;
using namespace loonglint;

static MCInst makeInst(unsigned Opcode, std::initializer_list<MCOperand> Operands) {
    MCInst Result;
    Result.setOpcode(Opcode);
    for (const MCOperand &Operand : Operands)
        Result.addOperand(Operand);
    return Result;
}

static MCInst immInst(int64_t Imm) {
    return makeInst(7, {MCOperand::createReg(MCRegister(2)), MCOperand::createImm(Imm)});
}

static void expectOps(ArrayRef<MCInst> Old, ArrayRef<MCInst> New,
                      std::initializer_list<DiffOp> Expected) {
    const SmallVector<DiffOp, 0> Ops = computeInstDiff(Old, New);
    ASSERT_EQ(Ops.size(), Expected.size());
    for (size_t I = 0; I < Expected.size(); ++I) {
        EXPECT_EQ(Ops[I].K, Expected.begin()[I].K);
        EXPECT_EQ(Ops[I].OldIdx, Expected.begin()[I].OldIdx);
        EXPECT_EQ(Ops[I].NewIdx, Expected.begin()[I].NewIdx);
    }
}

TEST(InstDiffTest, EmptyAgainstEmpty) {
    expectOps({}, {}, {});
}

TEST(InstDiffTest, EmptyAgainstContent) {
    expectOps({}, {immInst(42)}, {DiffOp{DiffOp::Kind::Insert, 0u, 0u}});
}

TEST(InstDiffTest, ContentAgainstEmpty) {
    const SmallVector<MCInst, 0> Old = {immInst(42)};
    expectOps(Old, {}, {DiffOp{DiffOp::Kind::Remove, 0u, 0u}});
}

TEST(InstDiffTest, IdenticalSequences) {
    const SmallVector<MCInst, 0> Old = {immInst(10), immInst(11)};
    const SmallVector<MCInst, 0> New = {immInst(10), immInst(11)};
    expectOps(Old, New, {DiffOp{DiffOp::Kind::Keep, 0u, 0u}, DiffOp{DiffOp::Kind::Keep, 1u, 1u}});
}

TEST(InstDiffTest, SingleLineChange) {
    const SmallVector<MCInst, 0> Old = {immInst(10)};
    const SmallVector<MCInst, 0> New = {
        makeInst(8, {MCOperand::createReg(MCRegister(2)), MCOperand::createImm(10)})};
    expectOps(Old, New,
              {DiffOp{DiffOp::Kind::Remove, 0u, 0u}, DiffOp{DiffOp::Kind::Insert, 0u, 0u}});
}

TEST(InstDiffTest, InteriorDeletionBetweenLongCommonRuns) {
    const SmallVector<MCInst, 0> Old = {immInst(10), immInst(11), immInst(12), immInst(13),
                                        immInst(99), immInst(14), immInst(15), immInst(16)};
    const SmallVector<MCInst, 0> New = {immInst(10), immInst(11), immInst(12), immInst(13),
                                        immInst(14), immInst(15), immInst(16)};
    expectOps(Old, New,
              {DiffOp{DiffOp::Kind::Keep, 0u, 0u}, DiffOp{DiffOp::Kind::Keep, 1u, 1u},
               DiffOp{DiffOp::Kind::Keep, 2u, 2u}, DiffOp{DiffOp::Kind::Keep, 3u, 3u},
               DiffOp{DiffOp::Kind::Remove, 4u, 0u}, DiffOp{DiffOp::Kind::Keep, 5u, 4u},
               DiffOp{DiffOp::Kind::Keep, 6u, 5u}, DiffOp{DiffOp::Kind::Keep, 7u, 6u}});
}

TEST(InstDiffTest, InteriorReplaceWithCommonSurroundings) {
    const SmallVector<MCInst, 0> Old = {immInst(10), immInst(99), immInst(30)};
    const SmallVector<MCInst, 0> New = {immInst(10), immInst(77), immInst(30)};
    expectOps(Old, New,
              {DiffOp{DiffOp::Kind::Keep, 0u, 0u}, DiffOp{DiffOp::Kind::Remove, 1u, 0u},
               DiffOp{DiffOp::Kind::Insert, 0u, 1u}, DiffOp{DiffOp::Kind::Keep, 2u, 2u}});
}

TEST(InstDiffTest, CommonPrefixOnly) {
    const SmallVector<MCInst, 0> Old = {immInst(10), immInst(99)};
    const SmallVector<MCInst, 0> New = {immInst(10), immInst(77)};
    expectOps(Old, New,
              {DiffOp{DiffOp::Kind::Keep, 0u, 0u}, DiffOp{DiffOp::Kind::Remove, 1u, 0u},
               DiffOp{DiffOp::Kind::Insert, 0u, 1u}});
}

TEST(InstDiffTest, CommonSuffixOnly) {
    const SmallVector<MCInst, 0> Old = {immInst(99), immInst(10)};
    const SmallVector<MCInst, 0> New = {immInst(77), immInst(10)};
    expectOps(Old, New,
              {DiffOp{DiffOp::Kind::Remove, 0u, 0u}, DiffOp{DiffOp::Kind::Insert, 0u, 0u},
               DiffOp{DiffOp::Kind::Keep, 1u, 1u}});
}

TEST(InstDiffTest, PermutationPairsNothing) {
    const SmallVector<MCInst, 0> Old = {immInst(10), immInst(11)};
    const SmallVector<MCInst, 0> New = {immInst(11), immInst(10)};
    expectOps(Old, New,
              {DiffOp{DiffOp::Kind::Remove, 0u, 0u}, DiffOp{DiffOp::Kind::Remove, 1u, 0u},
               DiffOp{DiffOp::Kind::Insert, 0u, 0u}, DiffOp{DiffOp::Kind::Insert, 0u, 1u}});
}

TEST(InstDiffTest, DuplicateCollapsesIntoKeepAndRemove) {
    const SmallVector<MCInst, 0> Old = {immInst(10), immInst(10)};
    const SmallVector<MCInst, 0> New = {immInst(10)};
    expectOps(Old, New, {DiffOp{DiffOp::Kind::Keep, 0u, 0u}, DiffOp{DiffOp::Kind::Remove, 1u, 0u}});
}

TEST(InstDiffTest, DuplicateExpandsIntoKeepAndInsert) {
    const SmallVector<MCInst, 0> Old = {immInst(10)};
    const SmallVector<MCInst, 0> New = {immInst(10), immInst(10)};
    expectOps(Old, New, {DiffOp{DiffOp::Kind::Keep, 0u, 0u}, DiffOp{DiffOp::Kind::Insert, 0u, 1u}});
}

TEST(InstDiffTest, InteriorCommonIsNotPaired) {
    const SmallVector<MCInst, 0> Old = {immInst(10), immInst(20), immInst(30)};
    const SmallVector<MCInst, 0> New = {immInst(40), immInst(20), immInst(50)};
    expectOps(Old, New,
              {DiffOp{DiffOp::Kind::Remove, 0u, 0u}, DiffOp{DiffOp::Kind::Remove, 1u, 0u},
               DiffOp{DiffOp::Kind::Remove, 2u, 0u}, DiffOp{DiffOp::Kind::Insert, 0u, 0u},
               DiffOp{DiffOp::Kind::Insert, 0u, 1u}, DiffOp{DiffOp::Kind::Insert, 0u, 2u}});
}

TEST(InstDiffTest, ShrinkKeepsCommonEnds) {
    const SmallVector<MCInst, 0> Old = {immInst(10), immInst(99), immInst(30)};
    const SmallVector<MCInst, 0> New = {immInst(10), immInst(30)};
    expectOps(Old, New,
              {DiffOp{DiffOp::Kind::Keep, 0u, 0u}, DiffOp{DiffOp::Kind::Remove, 1u, 0u},
               DiffOp{DiffOp::Kind::Keep, 2u, 1u}});
}

TEST(InstDiffTest, OperandCountMismatchIsNotEqual) {
    const SmallVector<MCInst, 0> Old = {
        makeInst(7, {MCOperand::createReg(MCRegister(2)), MCOperand::createImm(42)})};
    const SmallVector<MCInst, 0> New = {makeInst(7, {MCOperand::createReg(MCRegister(2))})};
    expectOps(Old, New,
              {DiffOp{DiffOp::Kind::Remove, 0u, 0u}, DiffOp{DiffOp::Kind::Insert, 0u, 0u}});
}

TEST(InstDiffTest, OpcodeMismatchIsNotEqual) {
    const SmallVector<MCInst, 0> Old = {immInst(10)};
    const SmallVector<MCInst, 0> New = {
        makeInst(8, {MCOperand::createReg(MCRegister(2)), MCOperand::createImm(10)})};
    expectOps(Old, New,
              {DiffOp{DiffOp::Kind::Remove, 0u, 0u}, DiffOp{DiffOp::Kind::Insert, 0u, 0u}});
}
