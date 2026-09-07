// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/RuleFilter.hpp"

#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/Error.h"

#include <string>
#include <utility>

using namespace llvm;

namespace loonglint {

Expected<RuleFilter> RuleFilter::create(ArrayRef<StringRef> ExcludePatterns) {
    RuleFilter RF;
    RF.Patterns.reserve(ExcludePatterns.size());

    for (const StringRef Pattern : ExcludePatterns) {
        if (Pattern.empty())
            return createStringError("empty exclusion pattern");

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
    return any_of(Patterns, [&](const Regex &R) { return R.match(RuleID); });
}

} // namespace loonglint
