// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/ScannedRegion.hpp"

#include "llvm/ADT/SmallVector.h"
#include "llvm/MC/MCInstrAnalysis.h"
#include "llvm/Support/Error.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <utility>

using namespace llvm;

namespace loonglint {

static constexpr unsigned CellSize = 4;

ScannedRegion::ScannedRegion(const DisassemblerTarget &DT, ArrayRef<uint8_t> Bytes,
                             uint64_t Address, size_t CellCount, uint64_t TrailingBytes)
    : DT(DT), Bytes(Bytes), Address(Address), CellCount(CellCount),
      Boundaries(static_cast<unsigned>(CellCount + 1)), TrailingBytes(TrailingBytes) {
    Boundaries.set(0);
    Boundaries.set(static_cast<unsigned>(CellCount));
}

Expected<ScannedRegion> ScannedRegion::create(const DisassemblerTarget &DT, ArrayRef<uint8_t> Bytes,
                                              uint64_t Address) {
    if (Bytes.size() > std::numeric_limits<uint64_t>::max() - Address)
        return createStringError("decode address range overflows");

    const size_t FullSize = Bytes.size() - Bytes.size() % CellSize;
    const size_t CellCount = FullSize / CellSize;
    if (CellCount > static_cast<size_t>(std::numeric_limits<unsigned>::max() - 1U))
        return createStringError("region contains too many instruction words");

    ScannedRegion Region(DT, Bytes, Address, CellCount, Bytes.size() - FullSize);
    const uint64_t EndAddress = Address + FullSize;
    MCInstrAnalysis &MIA = *DT.MIA;
    MIA.resetState();

    for (size_t CellIndex = 0; CellIndex < CellCount; ++CellIndex) {
        const size_t Offset = CellIndex * CellSize;
        const uint64_t InstAddress = Address + Offset;
        auto Decoded = DT.decodeInst(Bytes.slice(Offset, CellSize), InstAddress);
        if (!Decoded) {
            Region.OpaqueWords.set(static_cast<unsigned>(CellIndex));
            MIA.resetState();
            continue;
        }
        const auto &[Inst, Size] = *Decoded;

        const bool IsBranch = MIA.isBranch(Inst);
        const bool IsCall = MIA.isCall(Inst);
        const bool IsTerminator = MIA.isTerminator(Inst);

        uint64_t TargetAddress = 0;
        if ((IsBranch || IsCall) && MIA.evaluateBranch(Inst, InstAddress, 4, TargetAddress) &&
            TargetAddress >= Address && TargetAddress < EndAddress) {
            const uint64_t TargetOffset = TargetAddress - Address;
            if (TargetOffset % CellSize == 0)
                Region.Boundaries.set(static_cast<unsigned>(TargetOffset / CellSize));
        }

        if (IsCall || IsTerminator)
            Region.Boundaries.set(static_cast<unsigned>(CellIndex + 1));

        MIA.updateState(Inst, DT.MSTI.get(), InstAddress);
    }

    MIA.resetState();
    return Region;
}

RegionSummary ScannedRegion::summary() const {
    const uint64_t SkippedWords = OpaqueWords.count();
    return {CellCount - SkippedWords, SkippedWords, TrailingBytes, Address + CellCount * CellSize};
}

void ScannedRegion::forEachGap(GapHandler HandleGap) const {
    const auto EmitGap = [&](unsigned BeginCell, unsigned EndCell) {
        HandleGap(Address + static_cast<uint64_t>(BeginCell) * CellSize,
                  Address + static_cast<uint64_t>(EndCell) * CellSize);
    };

    std::optional<unsigned> GapBegin;
    unsigned GapEnd = 0;
    for (unsigned CellIndex : OpaqueWords) {
        if (!GapBegin) {
            GapBegin = CellIndex;
        } else if (CellIndex != GapEnd) {
            EmitGap(*GapBegin, GapEnd);
            GapBegin = CellIndex;
        }
        GapEnd = CellIndex + 1;
    }

    if (GapBegin)
        EmitGap(*GapBegin, GapEnd);
}

Expected<uint64_t> ScannedRegion::runRules(const RuleManager &Manager,
                                           FindingHandler HandleFinding) const {
    const unsigned MaxInstructionCount = Manager.maxInstructionCount();
    if (MaxInstructionCount == 0)
        return 0;

    SmallVector<Instruction> Window;
    Window.reserve(MaxInstructionCount);

    uint64_t FindingCount = 0;
    size_t NextCell = 0;
    for (size_t StartCell = 0; StartCell < CellCount; ++StartCell) {
        if (OpaqueWords.test(static_cast<unsigned>(StartCell))) {
            Window.clear();
            NextCell = StartCell + 1;
            continue;
        }

        while (Window.size() < MaxInstructionCount && NextCell < CellCount) {
            const unsigned NextCellBit = static_cast<unsigned>(NextCell);
            if (OpaqueWords.test(NextCellBit) || (!Window.empty() && Boundaries.test(NextCellBit)))
                break;

            const size_t Offset = NextCell * CellSize;
            const uint64_t InstructionAddress = Address + Offset;
            auto Decoded = DT.decodeInst(Bytes.slice(Offset, CellSize), InstructionAddress);
            if (!Decoded)
                return createStringError("instruction at 0x%llx became undecodable",
                                         static_cast<unsigned long long>(InstructionAddress));

            auto &[Inst, Size] = *Decoded;
            Window.emplace_back(InstructionAddress, Size, std::move(Inst));
            ++NextCell;
        }

        assert(!Window.empty() && Window.front().Address == Address + StartCell * CellSize &&
               "bounded instruction window lost synchronization");

        FindingCount += Manager.runWindow(Window, HandleFinding);

        Window.erase(Window.begin());
    }

    return FindingCount;
}

} // namespace loonglint
