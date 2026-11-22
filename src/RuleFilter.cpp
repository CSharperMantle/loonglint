// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/RuleFilter.hpp"

#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/Error.h"

#include <string>
#include <utility>

using namespace llvm;

namespace loonglint {

Expected<RuleFilter> RuleFilter::create(bool IsExclusion, ArrayRef<StringRef> Patterns) {
    RuleFilter RF;
    RF.IsExclusion = IsExclusion;
    RF.Patterns.reserve(Patterns.size());

    for (const StringRef Pattern : Patterns) {
        if (Pattern.empty())
            return createStringError("empty pattern");

        Regex R(Pattern);
        std::string ErrorMessage;
        if (!R.isValid(ErrorMessage))
            return createStringError(Twine("invalid regular expression '") + Pattern +
                                     "': " + ErrorMessage);
        RF.Patterns.emplace_back(std::move(R));
    }

    return RF;
}

bool RuleFilter::excludes(StringRef RuleID) const {
    const bool Hit = any_of(Patterns, [&](const Regex &R) { return R.match(RuleID); });
    return IsExclusion ? Hit : !Hit;
}

} // namespace loonglint
