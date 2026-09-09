// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/FunctionSymbols.hpp"

#include "llvm/ADT/STLExtras.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/Object/ELFObjectFile.h"

#include <cassert>
#include <cstdint>
#include <iterator>
#include <tuple>
#include <utility>

using namespace llvm;

namespace loonglint {

FunctionSymbols FunctionSymbols::create(const object::ELFObjectFileBase &TheELF,
                                        ArrayRef<std::pair<uint64_t, uint64_t>> TextRanges) {
    // Must be sorted by starting address.
    SmallVector<std::pair<uint64_t, uint64_t>> Ranges(TextRanges);
    stable_sort(Ranges,
                [](const std::pair<uint64_t, uint64_t> &LHS,
                   const std::pair<uint64_t, uint64_t> &RHS) { return LHS.first < RHS.first; });

    FunctionSymbols FS;
    for (const auto &Symbol : TheELF.symbols()) {
        if (Symbol.getELFType() != ELF::STT_FUNC)
            continue;

        Expected<StringRef> NameOrErr = Symbol.getName();
        if (!NameOrErr || NameOrErr->empty())
            continue;

        Expected<uint64_t> AddressOrErr = Symbol.getAddress();
        if (!AddressOrErr)
            continue;

        const uint64_t Size = Symbol.getSize();
        if (Size == 0)
            continue;

        const auto *const RangeEnd =
            upper_bound(Ranges, *AddressOrErr,
                        [](uint64_t Address, const std::pair<uint64_t, uint64_t> &Range) {
                            return Address < Range.first;
                        });
        if (RangeEnd == Ranges.begin())
            continue;
        const auto &Range = *std::prev(RangeEnd);
        if (*AddressOrErr < Range.first || *AddressOrErr >= Range.second)
            continue;

        FS.Entries.emplace_back(*AddressOrErr, Size, *NameOrErr);
    }

    stable_sort(FS.Entries, [](const Entry &LHS, const Entry &RHS) {
        return std::tie(LHS.Address, LHS.Name) < std::tie(RHS.Address, RHS.Name);
    });
    const auto *const UniqueEnd = unique(
        FS.Entries, [](const Entry &LHS, const Entry &RHS) { return LHS.Address == RHS.Address; });
    FS.Entries.erase(UniqueEnd, FS.Entries.end());

    assert(is_sorted(FS.Entries, [](const Entry &LHS,
                                    const Entry &RHS) { return LHS.Address < RHS.Address; }) &&
           "FunctionSymbols entries must be sorted by address");
    assert(adjacent_find(FS.Entries, [](const Entry &LHS,
                                        const Entry &RHS) { return LHS.Address == RHS.Address; }) ==
               FS.Entries.end() &&
           "FunctionSymbols entry addresses must be unique");

    return FS;
}

const FunctionSymbols::Entry *FunctionSymbols::find(uint64_t Address) const {
    const auto *const EntryEnd = upper_bound(
        Entries, Address, [](uint64_t LHS, const Entry &RHS) { return LHS < RHS.Address; });
    if (EntryEnd == Entries.begin())
        return nullptr;
    const Entry &TheEntry = *std::prev(EntryEnd);
    if (Address - TheEntry.Address >= TheEntry.Size)
        return nullptr;
    return &TheEntry;
}

} // namespace loonglint
