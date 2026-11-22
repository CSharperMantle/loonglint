// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LOONGLINT_RISCV_RISCVINSTR_HPP
#define LOONGLINT_RISCV_RISCVINSTR_HPP

#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCRegister.h"

#include <optional>

namespace loonglint::RISCV {

enum class HintKind {
    Standard,
    FutureStandard,
    Custom,
};

std::optional<HintKind> classifyHint(const llvm::MCInst &Inst);

std::optional<llvm::MCRegister> matchZeroWrite(const llvm::MCInst &Inst);

} // namespace loonglint::RISCV

#endif
