// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LOONGLINT_RULEFILTER_HPP
#define LOONGLINT_RULEFILTER_HPP

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/Regex.h"

namespace loonglint {

class RuleFilter final {
  public:
    static llvm::Expected<RuleFilter> create(llvm::ArrayRef<llvm::StringRef> ExcludePatterns);

    bool excludes(llvm::StringRef RuleID) const;

  private:
    RuleFilter() = default;

    llvm::SmallVector<llvm::Regex, 0> Patterns;
};

} // namespace loonglint

#endif
