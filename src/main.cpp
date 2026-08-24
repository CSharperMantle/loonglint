// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/DisassemblerTarget.hpp"
#include "loonglint/Rules.hpp"
#include "loonglint/ScannedRegion.hpp"

#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/BinaryFormat/Magic.h"
#include "llvm/Config/llvm-config.h"
#include "llvm/Object/ELFObjectFile.h"
#include "llvm/Object/ObjectFile.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/WithColor.h"
#include "llvm/Support/raw_ostream.h"

#include <cassert>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>

using namespace llvm;

namespace loonglint {

enum class InputFormat { Auto, Elf, Raw };
enum class ArchitectureOption { Unspecified, LoongArch32, LoongArch64 };

namespace opts {

cl::OptionCategory LoongLintCategory("LoongLint Options");

cl::opt<std::string> InputFile(cl::Positional, cl::desc("<input file>"),
                               cl::cat(LoongLintCategory));
cl::opt<InputFormat> InputFormat("input-format", cl::desc("Input format"),
                                 cl::init(InputFormat::Auto),
                                 cl::values(clEnumValN(InputFormat::Auto, "auto",
                                                       "Automatically detect file format"),
                                            clEnumValN(InputFormat::Elf, "elf", "ELF object"),
                                            clEnumValN(InputFormat::Raw, "raw", "Raw binary")),
                                 cl::cat(LoongLintCategory));
cl::opt<ArchitectureOption>
    Arch("arch", cl::desc("Architecture for raw input"), cl::init(ArchitectureOption::Unspecified),
         cl::values(clEnumValN(ArchitectureOption::LoongArch64, "loongarch64", "LoongArch64"),
                    clEnumValN(ArchitectureOption::LoongArch32, "loongarch32", "LoongArch32")),
         cl::cat(LoongLintCategory));
cl::opt<uint64_t> BaseAddress("base-address", cl::desc("Base address for raw input"), cl::init(0),
                              cl::value_desc("integer"), cl::cat(LoongLintCategory));
cl::opt<bool> ListChecks("list-checks", cl::desc("List available checks"),
                         cl::cat(LoongLintCategory));

} // namespace opts

} // namespace loonglint

using namespace loonglint;

enum class FindingLineKind { Removed, Added };

struct Totals {
    uint64_t Findings = 0;
    uint64_t DecodedInstructions = 0;
    uint64_t SkippedWords = 0;
    uint64_t TrailingBytes = 0;

    void add(const RegionSummary &Summary, uint64_t RegionFindings) {
        Findings += RegionFindings;
        DecodedInstructions += Summary.DecodedInstructions;
        SkippedWords += Summary.SkippedWords;
        TrailingBytes += Summary.TrailingBytes;
    }

    void add(const Totals &Other) {
        Findings += Other.Findings;
        DecodedInstructions += Other.DecodedInstructions;
        SkippedWords += Other.SkippedWords;
        TrailingBytes += Other.TrailingBytes;
    }
};

static void printVersion(raw_ostream &Output) {
    Output << "loonglint (LLVM " LLVM_VERSION_STRING ")\n";
}

static void printError(StringRef Message) {
    WithColor::error(errs(), "loonglint") << Message << '\n';
}

static bool validateOptions() {
    if (!opts::ListChecks && opts::InputFile.empty()) {
        printError("no input file");
        return false;
    }
    if (opts::InputFormat == InputFormat::Raw) {
        if (opts::Arch.getNumOccurrences() == 0) {
            printError("--arch is required with --input-format=raw");
            return false;
        }
    } else if (opts::InputFormat == InputFormat::Elf) {
        if (opts::Arch.getNumOccurrences() != 0) {
            printError("--arch is valid only with --input-format=raw");
            return false;
        }
        if (opts::BaseAddress.getNumOccurrences() != 0) {
            printError("--base-address is valid only with --input-format=raw");
            return false;
        }
    }
    return true;
}

static void printInstruction(DisassemblerTarget &Target, FindingLineKind Kind, uint64_t Address,
                             const MCInst &Inst) {
    switch (Kind) {
    case FindingLineKind::Removed: {
        WithColor LineColor(outs(), raw_ostream::RED);
        Target.setUseColor(LineColor.colorsEnabled());
        LineColor << "\t- " << format_hex(Address, 10);
        Target.printInst(Inst, Address, LineColor);
        break;
    }
    case FindingLineKind::Added: {
        WithColor LineColor(outs(), raw_ostream::GREEN);
        Target.setUseColor(LineColor.colorsEnabled());
        LineColor << "\t+ " << format_hex(Address, 10);
        Target.printInst(Inst, Address, LineColor);
        break;
    }
    }
    outs() << '\n';
}

static void printFinding(DisassemblerTarget &Target, StringRef RegionName,
                         const Finding &TheFinding) {
    const uint64_t Address = TheFinding.Instructions.front().Address;
    WithColor(outs(), raw_ostream::WHITE)
        << opts::InputFile << ':' << RegionName << ':' << format_hex(Address, 0) << ": "
        << TheFinding.MatchedRule.Description << ' ';
    WithColor(outs(), HighlightColor::Tag) << '[' << TheFinding.MatchedRule.Id << ']';
    outs() << '\n';

    for (const auto &I : TheFinding.Instructions)
        printInstruction(Target, FindingLineKind::Removed, I.Address, I.Inst);
    for (const auto &[II, MI] : enumerate(TheFinding.Match.Replacement))
        printInstruction(Target, FindingLineKind::Added, Address + 4 * II, MI);

    outs() << '\n';
}

static void printRegionWarnings(StringRef RegionName, const ScannedRegion &Region,
                                const RegionSummary &Summary) {
    Region.forEachGap([&](uint64_t Begin, uint64_t End) {
        WithColor::warning(errs(), "loonglint")
            << opts::InputFile << ':' << RegionName << ": skipped undecodable words in ["
            << format_hex(Begin, 0) << ", " << format_hex(End, 0) << ")\n";
    });

    if (Summary.TrailingBytes)
        WithColor::warning(errs(), "loonglint")
            << opts::InputFile << ':' << RegionName << ": ignored " << Summary.TrailingBytes
            << " trailing bytes at " << format_hex(Summary.TrailingAddress, 0) << '\n';
}

static Expected<Totals> lintRegion(DisassemblerTarget &Target, StringRef Name,
                                   ArrayRef<uint8_t> Bytes, uint64_t Address) {
    Expected<ScannedRegion> Region = ScannedRegion::create(Target, Bytes, Address);
    if (auto E = Region.takeError())
        return E;

    const RegionSummary Summary = Region->summary();
    printRegionWarnings(Name, *Region, Summary);

    Expected<uint64_t> FindingCount =
        Region->runRules(rules(), [&](const Finding &F) { printFinding(Target, Name, F); });
    if (auto E = FindingCount.takeError())
        return E;

    Totals Result;
    Result.add(Summary, *FindingCount);
    return Result;
}

static Error finish(const Totals &Total) {
    if (Total.DecodedInstructions == 0)
        return createStringError("no instructions decoded from '%s'", opts::InputFile.c_str());

    outs() << "findings: " << Total.Findings << "; skipped words: " << Total.SkippedWords
           << "; trailing bytes: " << Total.TrailingBytes << '\n';
    return Error::success();
}

static Expected<Totals> lintRaw(MemoryBufferRef Buffer) {
    if (opts::Arch == ArchitectureOption::Unspecified)
        return createStringError("--arch is required for raw input");

    const Architecture TheArchitecture = opts::Arch == ArchitectureOption::LoongArch32
                                             ? Architecture::LoongArch32
                                             : Architecture::LoongArch64;

    Expected<DisassemblerTarget> Target = DisassemblerTarget::create(TheArchitecture);
    if (auto E = Target.takeError())
        return E;

    return lintRegion(*Target, "<raw>", arrayRefFromStringRef(Buffer.getBuffer()),
                      opts::BaseAddress);
}

static Expected<Totals> lintELF(MemoryBufferRef Buffer) {
    Expected<std::unique_ptr<object::ObjectFile>> Object =
        object::ObjectFile::createObjectFile(Buffer);
    if (auto E = Object.takeError())
        return createStringError("cannot open '%s' as an object file: %s", opts::InputFile.c_str(),
                                 toString(std::move(E)).c_str());

    auto *const TheELF = dyn_cast<object::ELFObjectFileBase>(Object->get());
    if (!TheELF)
        return createStringError("unsupported input format for '%s': expected ELF",
                                 opts::InputFile.c_str());
    if (TheELF->getEMachine() != ELF::EM_LOONGARCH)
        return createStringError("unsupported ELF machine in '%s': expected LoongArch",
                                 opts::InputFile.c_str());
    if (!is_contained({ELF::ET_EXEC, ELF::ET_DYN}, TheELF->getEType()))
        return createStringError("unsupported ELF type in '%s': expected ET_EXEC or ET_DYN",
                                 opts::InputFile.c_str());

    assert(TheELF->isLittleEndian() && "big-endian ELF for LoongArch is so peculiar!");

    Expected<DisassemblerTarget> Target = DisassemblerTarget::create(
        TheELF->is64Bit() ? Architecture::LoongArch64 : Architecture::LoongArch32);
    if (auto E = Target.takeError())
        return E;

    Target->setABIVersion(TheELF->getEIdentABIVersion());

    Totals Total;
    bool HasCode = false;
    for (const auto &Section : Object->get()->sections()) {
        if (!Section.isText() || Section.getSize() == 0)
            continue;

        Expected<StringRef> Name = Section.getName();
        if (auto E = Name.takeError())
            return E;
        Expected<StringRef> Contents = Section.getContents();
        if (auto E = Contents.takeError())
            return E;
        if (Contents->empty())
            continue;

        HasCode = true;
        Expected<Totals> RegionTotal =
            lintRegion(*Target, Name->empty() ? "<unnamed>" : *Name,
                       arrayRefFromStringRef(*Contents), Section.getAddress());
        if (auto E = RegionTotal.takeError())
            return E;

        Total.add(*RegionTotal);
    }

    if (!HasCode)
        return createStringError("no non-empty executable sections in '%s'",
                                 opts::InputFile.c_str());
    return Total;
}

static Expected<Totals> lintInput() {
    ErrorOr<std::unique_ptr<MemoryBuffer>> Buffer =
        MemoryBuffer::getFile(opts::InputFile, /*IsText=*/false,
                              /*RequiresNullTerminator=*/false);
    if (!Buffer)
        return createStringError(Buffer.getError(), "cannot read '%s'", opts::InputFile.c_str());
    if ((*Buffer)->getBufferSize() == 0)
        return createStringError("input '%s' is empty", opts::InputFile.c_str());

    const MemoryBufferRef Ref = (*Buffer)->getMemBufferRef();

    switch (opts::InputFormat.getValue()) {
    case InputFormat::Auto:
        switch (identify_magic((*Buffer)->getBuffer())) {
        case file_magic::elf:
        case file_magic::elf_relocatable:
        case file_magic::elf_executable:
        case file_magic::elf_shared_object:
        case file_magic::elf_core:
            return lintELF(Ref);
        default:
            return lintRaw(Ref);
        }
    case InputFormat::Elf:
        return lintELF(Ref);
    case InputFormat::Raw:
        return lintRaw(Ref);
    }
}

int main(int argc, char **argv) {
    InitLLVM TheInitLLVM(argc, argv);

    cl::HideUnrelatedOptions({&opts::LoongLintCategory, &getColorCategory()});
    cl::SetVersionPrinter(printVersion);
    if (!cl::ParseCommandLineOptions(argc, argv, "Lint LoongArch machine code\n", &errs()))
        return 2;
    if (!validateOptions())
        return 2;

    if (opts::ListChecks) {
        for (const auto &R : rules())
            outs() << R.Id << ": " << R.Description << '\n';
        return 0;
    }

    LLVMInitializeLoongArchTargetInfo();
    LLVMInitializeLoongArchTargetMC();
    LLVMInitializeLoongArchDisassembler();

    Expected<Totals> Total = lintInput();
    if (auto E = Total.takeError()) {
        printError(toString(std::move(E)));
        return 2;
    }
    if (Error FinishError = finish(*Total)) {
        printError(toString(std::move(FinishError)));
        return 2;
    }
    return Total->Findings == 0 ? 0 : 1;
}
