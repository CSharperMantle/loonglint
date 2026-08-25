// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/DisassemblerTarget.hpp"

#include "llvm/MC/MCTargetOptions.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/TargetParser/Triple.h"

#include <cassert>
#include <memory>
#include <string>

using namespace llvm;

namespace loonglint {

Expected<DisassemblerTarget> DisassemblerTarget::create(Architecture TheArchitecture) {
    Triple TheTriple(TheArchitecture == Architecture::LoongArch32 ? "loongarch32" : "loongarch64");

    std::string Error;
    const Target *TheTarget = TargetRegistry::lookupTarget(TheTriple, Error);
    if (!TheTarget)
        return createStringError("cannot find target '%s': %s", TheTriple.str().c_str(),
                                 Error.c_str());

    DisassemblerTarget DT(TheArchitecture);

    DT.MRI.reset(TheTarget->createMCRegInfo(TheTriple));
    if (!DT.MRI)
        return createStringError("cannot initialize MCRegisterInfo");

    DT.MII.reset(TheTarget->createMCInstrInfo());
    if (!DT.MII)
        return createStringError("cannot initialize MCInstrInfo");

    DT.MAI.reset(TheTarget->createMCAsmInfo(*DT.MRI, TheTriple, MCTargetOptions()));
    if (!DT.MAI)
        return createStringError("cannot initialize MCAsmInfo");

    DT.MSTI.reset(TheTarget->createMCSubtargetInfo(TheTriple, "", ""));
    if (!DT.MSTI)
        return createStringError("cannot initialize MCSubtargetInfo");

    DT.Ctx = std::make_unique<MCContext>(TheTriple, *DT.MAI, *DT.MRI, *DT.MSTI);

    DT.MIA.reset(TheTarget->createMCInstrAnalysis(DT.MII.get()));
    if (!DT.MIA)
        return createStringError("cannot initialize MCInstrAnalysis");

    const unsigned AsmPrinterVariant = DT.MAI->getAssemblerDialect();
    DT.InstPrinter.reset(
        TheTarget->createMCInstPrinter(TheTriple, AsmPrinterVariant, *DT.MAI, *DT.MII, *DT.MRI));
    if (!DT.InstPrinter)
        return createStringError("cannot initialize MCInstPrinter");
    DT.InstPrinter->setPrintBranchImmAsAddress(true);
    DT.InstPrinter->setMCInstrAnalysis(DT.MIA.get());

    DT.Disasm.reset(TheTarget->createMCDisassembler(*DT.MSTI, *DT.Ctx));
    if (!DT.Disasm)
        return createStringError("cannot initialize MCDisassembler");

    return DT;
}

std::optional<MCInst> DisassemblerTarget::decodeInst(ArrayRef<uint8_t> Word,
                                                     uint64_t Address) const {
    if (Word.size() != 4)
        return std::nullopt;

    MCInst Instr;
    uint64_t Size = 0;
    if (Disasm->getInstruction(Instr, Size, Word, Address, nulls()) != MCDisassembler::Success)
        return std::nullopt;
    assert(Size == 4 && "non-4B LoongArch instruction is peculiar!");

    return Instr;
}

void DisassemblerTarget::setABIVersion(unsigned Version) {
    Disasm->setABIVersion(Version);
}

void DisassemblerTarget::setUseColor(bool UseColor) {
    InstPrinter->setUseColor(UseColor);
}

void DisassemblerTarget::printInst(const MCInst &MI, uint64_t Address, raw_ostream &Output) const {
    InstPrinter->printInst(&MI, Address, "", *MSTI, Output);
}

} // namespace loonglint
