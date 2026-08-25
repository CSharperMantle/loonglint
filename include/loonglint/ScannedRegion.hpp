// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LOONGLINT_SCANNEDREGION_HPP
#define LOONGLINT_SCANNEDREGION_HPP

#include "loonglint/DisassemblerTarget.hpp"
#include "loonglint/RuleManager.hpp"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/ADT/STLFunctionalExtras.h"
#include "llvm/ADT/SparseBitVector.h"
#include "llvm/Support/Error.h"

#include <cstddef>
#include <cstdint>

namespace loonglint {

using GapHandler = llvm::function_ref<void(uint64_t Begin, uint64_t End)>;

struct RegionSummary {
    uint64_t DecodedInstructions;
    uint64_t SkippedWords;
    uint64_t TrailingBytes;
    uint64_t TrailingAddress;
};

class ScannedRegion {
  public:
    static llvm::Expected<ScannedRegion> create(const DisassemblerTarget &Target,
                                                llvm::ArrayRef<uint8_t> Bytes, uint64_t Address);

    llvm::Expected<uint64_t> runRules(const RuleManager &Manager,
                                      FindingHandler HandleFinding) const;

    RegionSummary summary() const;
    void forEachGap(GapHandler HandleGap) const;

  private:
    ScannedRegion(const DisassemblerTarget &Target, llvm::ArrayRef<uint8_t> Bytes, uint64_t Address,
                  size_t WordCount, uint64_t TrailingBytes);

    const DisassemblerTarget &Target;
    llvm::ArrayRef<uint8_t> Bytes;
    uint64_t Address;
    size_t WordCount;
    // Set the corresponding bit to 1 to indicate an opaque word whose decoding failed. Indexed by
    // word.
    llvm::SparseBitVector<> OpaqueWords;
    // Set the corresponding bit to 1 to indicate a sequence boundary before a word. Indexed by
    // word, with the final bit denoting the region end.
    llvm::BitVector Boundaries;
    uint64_t TrailingBytes;
};

} // namespace loonglint

#endif
