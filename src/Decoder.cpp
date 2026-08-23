// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/Decoder.hpp"

#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCDisassembler/MCDisassembler.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/MCTargetOptions.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/TargetParser/Triple.h"

#include <limits>
#include <optional>
#include <string>

using namespace llvm;

namespace loonglint {

Expected<Decoder> Decoder::create(Architecture Arch) {
    Triple TheTriple(Arch == Architecture::LoongArch32 ? "loongarch32" : "loongarch64");

    std::string Error;
    const Target *T = TargetRegistry::lookupTarget(TheTriple, Error);
    if (!T)
        return createStringError("cannot find target '%s': %s", TheTriple.str().c_str(),
                                 Error.c_str());

    Decoder D;

    D.MRI.reset(T->createMCRegInfo(TheTriple));
    if (!D.MRI)
        return createStringError("cannot initialize MCRegisterInfo");
    D.MAI.reset(T->createMCAsmInfo(*D.MRI, TheTriple, MCTargetOptions()));
    if (!D.MAI)
        return createStringError("cannot initialize MCAsmInfo");
    D.MSTI.reset(T->createMCSubtargetInfo(TheTriple, "", ""));
    if (!D.MSTI)
        return createStringError("cannot initialize MCSubtargetInfo");
    D.Ctx = std::make_unique<MCContext>(TheTriple, *D.MAI, *D.MRI, *D.MSTI);
    D.Disasm.reset(T->createMCDisassembler(*D.MSTI, *D.Ctx));
    if (!D.Disasm)
        return createStringError("cannot initialize MCDisassembler");

    return D;
}

Expected<Decoder::Result> Decoder::decode(ArrayRef<uint8_t> Bytes, uint64_t Address) const {
    if (Bytes.size() > std::numeric_limits<uint64_t>::max() - Address)
        return createStringError("decode address range overflows");

    Result R;
    const size_t FullSize = Bytes.size() - Bytes.size() % 4;
    std::optional<uint64_t> GapStart;

    auto finishGap = [&](uint64_t End) {
        if (GapStart)
            R.Gaps.push_back({*GapStart, End});
        GapStart.reset();
    };

    for (std::size_t Offset = 0; Offset < FullSize; Offset += 4) {
        const uint64_t InstructionAddress = Address + Offset;
        MCInst Instruction;
        uint64_t Size = 0;
        switch (Disasm->getInstruction(Instruction, Size, Bytes.slice(Offset, 4),
                                       InstructionAddress, nulls())) {
        case MCDisassembler::Success:
            assert(Size == 4 && "non-4B LoongArch instructions are peculiar!");
            finishGap(InstructionAddress);
            ++R.DecodedInstructions;
            break;
        default:
            if (!GapStart)
                GapStart = InstructionAddress;
            ++R.SkippedWords;
            break;
        }
    }

    finishGap(Address + FullSize);
    R.TrailingBytes = Bytes.size() - FullSize;
    return R;
}

void Decoder::setABIVersion(unsigned Version) {
    Disasm->setABIVersion(Version);
}

} // namespace loonglint
