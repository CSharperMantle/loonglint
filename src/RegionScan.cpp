// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/RegionScan.hpp"

#include "llvm/ADT/SmallVector.h"
#include "llvm/MC/MCInst.h"
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

ScannedRegion::ScannedRegion(const DisassemblerTarget &Target, ArrayRef<uint8_t> Bytes,
                             uint64_t Address, size_t WordCount, uint64_t TrailingBytes)
    : Target(Target), Bytes(Bytes), Address(Address), WordCount(WordCount),
      Boundaries(static_cast<unsigned>(WordCount + 1)), TrailingBytes(TrailingBytes) {
    Boundaries.set(0);
    Boundaries.set(static_cast<unsigned>(WordCount));
}

Expected<ScannedRegion> ScannedRegion::create(const DisassemblerTarget &Target,
                                              ArrayRef<uint8_t> Bytes, uint64_t Address) {
    if (Bytes.size() > std::numeric_limits<uint64_t>::max() - Address)
        return createStringError("decode address range overflows");

    const size_t FullSize = Bytes.size() - Bytes.size() % 4;
    const size_t WordCount = FullSize / 4;
    if (WordCount > static_cast<size_t>(std::numeric_limits<unsigned>::max() - 1U))
        return createStringError("region contains too many instruction words");

    ScannedRegion Region(Target, Bytes, Address, WordCount, Bytes.size() - FullSize);
    const uint64_t EndAddress = Address + FullSize;
    MCInstrAnalysis &MIA = *Target.MIA;
    MIA.resetState();

    for (size_t WordIndex = 0; WordIndex < WordCount; ++WordIndex) {
        const size_t Offset = WordIndex * 4;
        const uint64_t InstAddress = Address + Offset;
        std::optional<MCInst> Inst = Target.decodeInst(Bytes.slice(Offset, 4), InstAddress);
        if (!Inst) {
            Region.OpaqueWords.set(static_cast<unsigned>(WordIndex));
            MIA.resetState();
            continue;
        }

        const bool IsBranch = MIA.isBranch(*Inst);
        const bool IsCall = MIA.isCall(*Inst);
        const bool IsTerminator = MIA.isTerminator(*Inst);

        uint64_t TargetAddress = 0;
        if ((IsBranch || IsCall) && MIA.evaluateBranch(*Inst, InstAddress, 4, TargetAddress) &&
            TargetAddress >= Address && TargetAddress < EndAddress) {
            const uint64_t TargetOffset = TargetAddress - Address;
            if (TargetOffset % 4 == 0)
                Region.Boundaries.set(static_cast<unsigned>(TargetOffset / 4));
        }

        if (IsCall || IsTerminator)
            Region.Boundaries.set(static_cast<unsigned>(WordIndex + 1));

        MIA.updateState(*Inst, Target.MSTI.get(), InstAddress);
    }

    MIA.resetState();
    return Region;
}

RegionSummary ScannedRegion::summary() const {
    const uint64_t SkippedWords = OpaqueWords.count();
    return {WordCount - SkippedWords, SkippedWords, TrailingBytes, Address + WordCount * 4};
}

void ScannedRegion::forEachGap(GapHandler HandleGap) const {
    size_t WordIndex = 0;
    while (WordIndex < WordCount) {
        if (!OpaqueWords.test(static_cast<unsigned>(WordIndex))) {
            ++WordIndex;
            continue;
        }

        const size_t GapBegin = WordIndex;
        while (WordIndex < WordCount && OpaqueWords.test(static_cast<unsigned>(WordIndex)))
            ++WordIndex;

        HandleGap(Address + GapBegin * 4, Address + WordIndex * 4);
    }
}

Expected<uint64_t> ScannedRegion::runRules(ArrayRef<Rule> Rules,
                                           FindingHandler HandleFinding) const {
    const unsigned MaximumInstructionCount = maxInstCount(Rules);

    if (Rules.empty())
        return 0;

    SmallVector<Instruction> Window;
    Window.reserve(MaximumInstructionCount);

    uint64_t FindingCount = 0;
    size_t NextWord = 0;
    for (size_t StartWord = 0; StartWord < WordCount; ++StartWord) {
        if (OpaqueWords.test(static_cast<unsigned>(StartWord))) {
            Window.clear();
            NextWord = StartWord + 1;
            continue;
        }

        while (Window.size() < MaximumInstructionCount && NextWord < WordCount) {
            const unsigned NextBit = static_cast<unsigned>(NextWord);
            if (OpaqueWords.test(NextBit) || (!Window.empty() && Boundaries.test(NextBit)))
                break;

            const size_t Offset = NextWord * 4;
            const uint64_t InstructionAddress = Address + Offset;
            std::optional<MCInst> Inst =
                Target.decodeInst(Bytes.slice(Offset, 4), InstructionAddress);
            if (!Inst)
                return createStringError("instruction at 0x%llx became undecodable",
                                         static_cast<unsigned long long>(InstructionAddress));

            Window.push_back({InstructionAddress, std::move(*Inst)});
            ++NextWord;
        }

        assert(!Window.empty() && Window.front().Address == Address + StartWord * 4 &&
               "bounded instruction window lost synchronization");

        for (const auto &R : Rules) {
            if (R.InstructionCount > Window.size())
                continue;

            ArrayRef<Instruction> Instructions(Window.data(), R.InstructionCount);
            if (std::optional<RuleMatch> Match = R.Run(Instructions, {Target.Arch, *Target.MIA})) {
                HandleFinding({R, Instructions, *Match});
                ++FindingCount;
            }
        }

        Window.erase(Window.begin());
    }

    return FindingCount;
}

} // namespace loonglint
