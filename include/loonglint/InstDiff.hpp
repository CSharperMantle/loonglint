// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LOONGLINT_INSTDIFF_HPP
#define LOONGLINT_INSTDIFF_HPP

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/MC/MCInst.h"

namespace loonglint {

struct DiffOp {
    enum class Kind { Keep, Remove, Insert };

    Kind K;
    unsigned OldIdx = 0;
    unsigned NewIdx = 0;
};

bool isSameInst(const llvm::MCInst &LHS, const llvm::MCInst &RHS);

llvm::SmallVector<DiffOp, 0> computeInstDiff(llvm::ArrayRef<llvm::MCInst> Old,
                                             llvm::ArrayRef<llvm::MCInst> New);

} // namespace loonglint

#endif
