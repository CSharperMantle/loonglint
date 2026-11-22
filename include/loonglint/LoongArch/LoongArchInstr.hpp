// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LOONGLINT_LOONGARCH_LOONGARCHINSTR_HPP
#define LOONGLINT_LOONGARCH_LOONGARCHINSTR_HPP

#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCRegister.h"

#include <optional>

namespace loonglint::LoongArch {

std::optional<llvm::MCRegister> matchZeroWrite(const llvm::MCInst &Inst);

} // namespace loonglint::LoongArch

#endif
