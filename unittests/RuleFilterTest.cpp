// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/RuleFilter.hpp"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Error.h"

#include "gtest/gtest.h"

#include <string>
#include <utility>

using namespace llvm;
using namespace loonglint;

static void expectCreateError(bool IsExclusion, ArrayRef<StringRef> Patterns,
                              StringRef MessagePart) {
    Expected<RuleFilter> MaybeFilter = RuleFilter::create(IsExclusion, Patterns);
    EXPECT_FALSE(static_cast<bool>(MaybeFilter));
    const std::string Message = toString(MaybeFilter.takeError());
    EXPECT_NE(Message.find(MessagePart), std::string::npos) << Message;
}

TEST(RuleFilterTest, EmptyFilterExcludesNothing) {
    Expected<RuleFilter> MaybeFilter = RuleFilter::create(true, {});
    ASSERT_TRUE(static_cast<bool>(MaybeFilter));
    EXPECT_FALSE(MaybeFilter->excludes("integer/nop"));
    EXPECT_FALSE(MaybeFilter->excludes("memory/load-extend"));
}

TEST(RuleFilterTest, UnanchoredSubstringMatch) {
    Expected<RuleFilter> MaybeFilter = RuleFilter::create(true, {"memory/"});
    ASSERT_TRUE(static_cast<bool>(MaybeFilter));
    EXPECT_TRUE(MaybeFilter->excludes("memory/load-extend"));
    EXPECT_TRUE(MaybeFilter->excludes("memory/unsigned-load-pick"));
    EXPECT_FALSE(MaybeFilter->excludes("integer/nop"));
    EXPECT_FALSE(MaybeFilter->excludes("control/branch-to-next"));
}

TEST(RuleFilterTest, FullAnchorMatchesExactly) {
    Expected<RuleFilter> MaybeFilter = RuleFilter::create(true, {"^integer/nop$"});
    ASSERT_TRUE(static_cast<bool>(MaybeFilter));
    EXPECT_TRUE(MaybeFilter->excludes("integer/nop"));
    EXPECT_FALSE(MaybeFilter->excludes("integer/nope"));
    EXPECT_FALSE(MaybeFilter->excludes("control/integer/nop"));
}

TEST(RuleFilterTest, StartAnchorDoesNotMatchEmbedded) {
    Expected<RuleFilter> MaybeFilter = RuleFilter::create(true, {"^nop"});
    ASSERT_TRUE(static_cast<bool>(MaybeFilter));
    EXPECT_FALSE(MaybeFilter->excludes("integer/nop"));
}

TEST(RuleFilterTest, PatternsFormAUnion) {
    Expected<RuleFilter> MaybeFilter = RuleFilter::create(true, {"^integer/nop$", "control/"});
    ASSERT_TRUE(static_cast<bool>(MaybeFilter));
    EXPECT_TRUE(MaybeFilter->excludes("integer/nop"));
    EXPECT_TRUE(MaybeFilter->excludes("control/degenerate-branch"));
    EXPECT_FALSE(MaybeFilter->excludes("integer/addi-pair"));
    EXPECT_FALSE(MaybeFilter->excludes("memory/indexed-load"));
}

TEST(RuleFilterTest, SupportsEREMetacharacters) {
    Expected<RuleFilter> MaybeFilter = RuleFilter::create(true, {"integer/(addi-pair|nop)"});
    ASSERT_TRUE(static_cast<bool>(MaybeFilter));
    EXPECT_TRUE(MaybeFilter->excludes("integer/addi-pair"));
    EXPECT_TRUE(MaybeFilter->excludes("integer/nop"));
    EXPECT_FALSE(MaybeFilter->excludes("integer/shift-chain"));
}

TEST(RuleFilterTest, MatchIsCaseSensitive) {
    Expected<RuleFilter> MaybeFilter = RuleFilter::create(true, {"MEMORY/", "Control/"});
    ASSERT_TRUE(static_cast<bool>(MaybeFilter));
    EXPECT_FALSE(MaybeFilter->excludes("memory/load-extend"));
    EXPECT_FALSE(MaybeFilter->excludes("control/branch-to-next"));
}

TEST(RuleFilterTest, FilterSurvivesMove) {
    Expected<RuleFilter> MaybeFilter = RuleFilter::create(true, {"control/"});
    ASSERT_TRUE(static_cast<bool>(MaybeFilter));
    RuleFilter Filter = std::move(*MaybeFilter);
    EXPECT_TRUE(Filter.excludes("control/branch-to-next"));
    EXPECT_FALSE(Filter.excludes("integer/nop"));
}

TEST(RuleFilterTest, EmptyPatternIsRejected) {
    expectCreateError(true, {""}, "empty pattern");
    expectCreateError(false, {""}, "empty pattern");
}

TEST(RuleFilterTest, InvalidPatternIsRejected) {
    expectCreateError(true, {"["}, "invalid regular expression");
    expectCreateError(false, {"["}, "invalid regular expression");
}

TEST(RuleFilterTest, InvalidPatternIsRejectedAmongValidOnes) {
    expectCreateError(true, {"control/", "["}, "invalid regular expression");
    expectCreateError(false, {"control/", "["}, "invalid regular expression");
}

TEST(RuleFilterTest, EmptyIncludeFilterExcludesEverything) {
    Expected<RuleFilter> MaybeFilter = RuleFilter::create(false, {});
    ASSERT_TRUE(static_cast<bool>(MaybeFilter));
    EXPECT_TRUE(MaybeFilter->excludes("integer/nop"));
    EXPECT_TRUE(MaybeFilter->excludes("memory/load-extend"));
}

TEST(RuleFilterTest, InclusionKeepsOnlyMatches) {
    Expected<RuleFilter> MaybeFilter = RuleFilter::create(false, {"memory/"});
    ASSERT_TRUE(static_cast<bool>(MaybeFilter));
    EXPECT_FALSE(MaybeFilter->excludes("memory/load-extend"));
    EXPECT_FALSE(MaybeFilter->excludes("memory/unsigned-load-pick"));
    EXPECT_TRUE(MaybeFilter->excludes("integer/nop"));
    EXPECT_TRUE(MaybeFilter->excludes("control/branch-to-next"));
}

TEST(RuleFilterTest, InclusionPatternsFormAUnion) {
    Expected<RuleFilter> MaybeFilter = RuleFilter::create(false, {"^integer/nop$", "control/"});
    ASSERT_TRUE(static_cast<bool>(MaybeFilter));
    EXPECT_FALSE(MaybeFilter->excludes("integer/nop"));
    EXPECT_FALSE(MaybeFilter->excludes("control/degenerate-branch"));
    EXPECT_TRUE(MaybeFilter->excludes("integer/addi-pair"));
    EXPECT_TRUE(MaybeFilter->excludes("memory/indexed-load"));
}
