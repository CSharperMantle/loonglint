// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/InstDiff.hpp"

#include "llvm/ADT/STLExtras.h"
#include "llvm/MC/MCInst.h"

#include <algorithm>
#include <cstddef>

using namespace llvm;

namespace loonglint {

static bool operator==(const MCOperand &LHS, const MCOperand &RHS) {
    if (LHS.isValid() != RHS.isValid())
        return false;
    if (LHS.isReg())
        return RHS.isReg() && LHS.getReg() == RHS.getReg();
    if (LHS.isImm())
        return RHS.isImm() && LHS.getImm() == RHS.getImm();
    if (LHS.isSFPImm())
        return RHS.isSFPImm() && LHS.getSFPImm() == RHS.getSFPImm();
    if (LHS.isDFPImm())
        return RHS.isDFPImm() && LHS.getDFPImm() == RHS.getDFPImm();
    if (LHS.isExpr())
        return RHS.isExpr() && LHS.getExpr() == RHS.getExpr();
    if (LHS.isInst())
        return RHS.isInst() && LHS.getInst() == RHS.getInst();
    return !RHS.isValid();
}

bool isSameInst(const MCInst &LHS, const MCInst &RHS) {
    if (LHS.getOpcode() != RHS.getOpcode() || LHS.getFlags() != RHS.getFlags() ||
        LHS.getNumOperands() != RHS.getNumOperands())
        return false;
    return all_of_zip(LHS.getOperands(), RHS.getOperands(),
                      [](const MCOperand &LO, const MCOperand &RO) { return LO == RO; });
}

SmallVector<DiffOp, 0> computeInstDiff(ArrayRef<MCInst> Old, ArrayRef<MCInst> New) {
    SmallVector<DiffOp, 0> Ops;

    const size_t MinLength = std::min(Old.size(), New.size());

    size_t Prefix = 0;
    while (Prefix < MinLength && isSameInst(Old[Prefix], New[Prefix]))
        ++Prefix;

    size_t Suffix = 0;
    while (Suffix < MinLength - Prefix &&
           isSameInst(Old[Old.size() - 1 - Suffix], New[New.size() - 1 - Suffix]))
        ++Suffix;

    const size_t OldMiddleEnd = Old.size() - Suffix;
    const size_t NewMiddleEnd = New.size() - Suffix;

    for (size_t I = 0; I < Prefix; ++I)
        Ops.emplace_back(DiffOp::Kind::Keep, static_cast<unsigned>(I), static_cast<unsigned>(I));

    for (size_t I = Prefix; I < OldMiddleEnd; ++I)
        Ops.emplace_back(DiffOp::Kind::Remove, static_cast<unsigned>(I), 0);

    for (size_t I = Prefix; I < NewMiddleEnd; ++I)
        Ops.emplace_back(DiffOp::Kind::Insert, 0, static_cast<unsigned>(I));

    for (size_t I = 0; I < Suffix; ++I) {
        const unsigned OldIdx = static_cast<unsigned>(Old.size() - Suffix + I);
        const unsigned NewIdx = static_cast<unsigned>(New.size() - Suffix + I);
        Ops.emplace_back(DiffOp::Kind::Keep, OldIdx, NewIdx);
    }

    return Ops;
}

} // namespace loonglint
