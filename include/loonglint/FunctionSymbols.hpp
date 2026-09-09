// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LOONGLINT_FUNCTIONSYMBOLS_HPP
#define LOONGLINT_FUNCTIONSYMBOLS_HPP

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Object/ELFObjectFile.h"

#include <cstdint>
#include <utility>

namespace loonglint {

class FunctionSymbols final {
  public:
    struct Entry {
        uint64_t Address;
        uint64_t Size;
        llvm::StringRef Name;
    };

    // TextRanges: [start, end).
    static FunctionSymbols create(const llvm::object::ELFObjectFileBase &TheELF,
                                  llvm::ArrayRef<std::pair<uint64_t, uint64_t>> TextRanges);

    const Entry *find(uint64_t Address) const;

  private:
    FunctionSymbols() = default;

    llvm::SmallVector<Entry, 0> Entries;
};

} // namespace loonglint

#endif
