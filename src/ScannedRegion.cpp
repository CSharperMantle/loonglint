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

ScannedRegion::ScannedRegion(const DisassemblerTarget &DT, ArrayRef<uint8_t> Bytes,
                             uint64_t Address, size_t CellCount, uint64_t TrailingBytes)
    : DT(DT), Bytes(Bytes), Address(Address), CellCount(CellCount), Starts(CellCount),
      Boundaries(static_cast<unsigned>(CellCount + 1)), TrailingBytes(TrailingBytes) {
    Boundaries.set(0);
    Boundaries.set(static_cast<unsigned>(CellCount));
}

Expected<ScannedRegion> ScannedRegion::create(const DisassemblerTarget &DT, ArrayRef<uint8_t> Bytes,
                                              uint64_t Address) {
    if (Bytes.size() > std::numeric_limits<uint64_t>::max() - Address)
        return createStringError("decode address range overflows");

    const size_t FullSize = Bytes.size() - Bytes.size() % DT.getCellSize();
    const size_t CellCount = FullSize / DT.getCellSize();
    if (CellCount > static_cast<size_t>(std::numeric_limits<unsigned>::max() - 1U))
        return createStringError("region contains too many instruction words");

    ScannedRegion Region(DT, Bytes, Address, CellCount, Bytes.size() - FullSize);
    const uint64_t EndAddress = Address + FullSize;
    MCInstrAnalysis &MIA = *DT.MIA;
    MIA.resetState();

    size_t Pos = 0;
    for (unsigned CellIndex = 0; Pos + DT.getCellSize() <= FullSize; CellIndex++) {
        const uint64_t InstAddress = Address + Pos;
        auto Decoded = DT.decodeInst(Bytes.slice(Pos), InstAddress);
        if (!Decoded) {
            Region.OpaqueWords.set(CellIndex);
            MIA.resetState();
            Pos += DT.getCellSize();
            continue;
        }
        const auto &[Inst, Size] = *Decoded;

        // The current instruction decodes well. Mark it as a start.
        Region.Starts.set(CellIndex);

        const bool IsBranch = MIA.isBranch(Inst);
        const bool IsCall = MIA.isCall(Inst);
        const bool IsTerminator = MIA.isTerminator(Inst);

        uint64_t TargetAddress = 0;
        if ((IsBranch || IsCall) && MIA.evaluateBranch(Inst, InstAddress, Size, TargetAddress) &&
            TargetAddress >= Address && TargetAddress < EndAddress) {
            const uint64_t TargetOffset = TargetAddress - Address;
            if (TargetOffset % DT.getCellSize() == 0)
                Region.Boundaries.set(static_cast<unsigned>(TargetOffset / DT.getCellSize()));
        }

        if (IsCall || IsTerminator)
            Region.Boundaries.set(CellIndex + 1);

        MIA.updateState(Inst, DT.MSTI.get(), InstAddress);
        Pos += Size;
    }

    MIA.resetState();
    return Region;
}

RegionSummary ScannedRegion::summarize() const {
    const uint64_t SkippedWords = OpaqueWords.count();
    return {CellCount - SkippedWords, SkippedWords, TrailingBytes,
            Address + CellCount * DT.getCellSize()};
}

void ScannedRegion::forEachGap(GapHandler HandleGap) const {
    const auto EmitGap = [&](unsigned BeginCell, unsigned EndCell) {
        HandleGap(Address + static_cast<uint64_t>(BeginCell) * DT.getCellSize(),
                  Address + static_cast<uint64_t>(EndCell) * DT.getCellSize());
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
    const unsigned MaxInstCount = Manager.getMaxInstCount();
    if (MaxInstCount == 0)
        return 0;

    SmallVector<Instruction> Window;
    Window.reserve(MaxInstCount);

    uint64_t FindingCount = 0;
    size_t NextCell = 0;
    for (size_t StartCell = 0; StartCell < CellCount; ++StartCell) {
        if (OpaqueWords.test(static_cast<unsigned>(StartCell))) {
            Window.clear();
            NextCell = StartCell + 1;
            continue;
        }
        if (!Starts.test(static_cast<unsigned>(StartCell)))
            // |StartCell| is not a start of an instruction. Skip this cell.
            continue;

        while (Window.size() < MaxInstCount && NextCell < CellCount) {
            const unsigned NextCellBit = static_cast<unsigned>(NextCell);
            if (OpaqueWords.test(NextCellBit) || (!Window.empty() && Boundaries.test(NextCellBit)))
                break;
            if (!Starts.test(NextCellBit)) {
                // The current instruction has not finished yet.
                ++NextCell;
                continue;
            }

            const size_t Offset = NextCell * DT.getCellSize();
            const uint64_t InstructionAddress = Address + Offset;
            auto Decoded = DT.decodeInst(Bytes.slice(Offset), InstructionAddress);
            if (!Decoded)
                return createStringError("instruction at 0x%llx became undecodable",
                                         static_cast<unsigned long long>(InstructionAddress));

            auto &[Inst, Size] = *Decoded;
            Window.emplace_back(InstructionAddress, Size, std::move(Inst));
            ++NextCell;
        }

        assert(!Window.empty() &&
               Window.front().Address == Address + StartCell * DT.getCellSize() &&
               "bounded instruction window lost synchronization");

        FindingCount += Manager.runWindow(Window, HandleFinding);

        Window.erase(Window.begin());
    }

    return FindingCount;
}

} // namespace loonglint
