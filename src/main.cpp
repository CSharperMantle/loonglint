// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/Decoder.hpp"

#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/BinaryFormat/ELF.h"
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
using namespace loonglint;

enum class InputFormat { Auto, Elf, Raw };

namespace loonglint {

namespace opts {

cl::OptionCategory LoongLintCategory("loonglint options");

cl::opt<std::string> InputFile(cl::Positional, cl::desc("<input file>"), cl::Optional,
                               cl::cat(LoongLintCategory));
cl::opt<InputFormat>
    InputFormat("input-format", cl::desc("Input format"), cl::init(InputFormat::Auto),
                cl::values(clEnumValN(InputFormat::Auto, "auto", "Recognize ELF input"),
                           clEnumValN(InputFormat::Elf, "elf", "Require ELF input"),
                           clEnumValN(InputFormat::Raw, "raw", "Treat input as raw code")),
                cl::cat(LoongLintCategory));
cl::opt<Architecture>
    Architecture("arch", cl::desc("Architecture for raw input"),
                 cl::init(Architecture::Unspecified),
                 cl::values(clEnumValN(Architecture::LoongArch64, "loongarch64", "LoongArch64"),
                            clEnumValN(Architecture::LoongArch32, "loongarch32", "LoongArch32")),
                 cl::cat(LoongLintCategory));
cl::opt<uint64_t> BaseAddress("base-address", cl::desc("Base address for raw input"), cl::init(0),
                              cl::value_desc("integer"), cl::cat(LoongLintCategory));
cl::opt<bool> ListChecks("list-checks", cl::desc("List available checks"),
                         cl::cat(LoongLintCategory));
cl::opt<bool> Verbose("verbose", cl::desc("Show verbose findings"), cl::cat(LoongLintCategory));
cl::alias VerboseShort("v", cl::NotHidden, cl::desc("Alias for --verbose"), cl::aliasopt(Verbose),
                       cl::cat(LoongLintCategory));

} // namespace opts

} // namespace loonglint

struct Totals {
    uint64_t DecodedInstructions = 0;
    uint64_t SkippedWords = 0;
    uint64_t TrailingBytes = 0;

    void add(const Decoder::Result &Result) {
        DecodedInstructions += Result.DecodedInstructions;
        SkippedWords += Result.SkippedWords;
        TrailingBytes += Result.TrailingBytes;
    }
};

static void printVersion(raw_ostream &Output) {
    Output << "loonglint (LLVM " LLVM_VERSION_STRING ")\n";
}

static void printError(StringRef Message) {
    WithColor::error(errs(), "loonglint") << Message << '\n';
}

static bool validateOptions() {
    if (opts::ListChecks)
        return true;
    if (opts::InputFile.empty()) {
        printError("no input file");
        return false;
    }
    if (opts::InputFormat == InputFormat::Raw) {
        if (opts::Architecture.getNumOccurrences() == 0) {
            printError("--arch is required with --input-format=raw");
            return false;
        }
    } else {
        if (opts::Architecture.getNumOccurrences() != 0) {
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

static Expected<Decoder::Result> decodeRegion(Decoder &D, StringRef Name, ArrayRef<uint8_t> Bytes,
                                              uint64_t Address) {
    Expected<Decoder::Result> Result = D.decode(Bytes, Address);
    if (auto E = Result.takeError())
        return E;

    if (opts::Verbose) {
        for (const Decoder::Gap &Gap : Result->Gaps)
            WithColor::warning(errs(), "loonglint")
                << opts::InputFile << ':' << Name << ": skipped undecodable words in ["
                << format_hex(Gap.Begin, 0) << ", " << format_hex(Gap.End, 0) << ")\n";

        if (Result->TrailingBytes)
            WithColor::warning(errs(), "loonglint")
                << opts::InputFile << ':' << Name << ": ignored " << Result->TrailingBytes
                << " trailing bytes at "
                << format_hex(Address + Bytes.size() - Result->TrailingBytes, 0) << '\n';
    }

    return Result;
}

static Error finish(const Totals &Totals) {
    if (Totals.DecodedInstructions == 0)
        return createStringError("no instructions decoded from '%s'", opts::InputFile.c_str());
    outs() << "findings: 0; skipped words: " << Totals.SkippedWords
           << "; trailing bytes: " << Totals.TrailingBytes << '\n';
    return Error::success();
}

static Error lintRaw(MemoryBufferRef Buffer) {
    Expected<Decoder> D = Decoder::create(opts::Architecture);
    if (auto E = D.takeError())
        return E;

    Expected<Decoder::Result> Result =
        decodeRegion(*D, "raw", arrayRefFromStringRef(Buffer.getBuffer()), opts::BaseAddress);
    if (auto E = Result.takeError())
        return E;

    Totals Total;
    Total.add(*Result);
    return finish(Total);
}

static Error lintELF(MemoryBufferRef Buffer) {
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

    Expected<Decoder> D =
        Decoder::create(TheELF->is64Bit() ? Architecture::LoongArch64 : Architecture::LoongArch32);
    if (auto E = D.takeError())
        return E;

    D->setABIVersion(TheELF->getEIdentABIVersion());

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
        Expected<Decoder::Result> Result =
            decodeRegion(*D, Name->empty() ? "<unnamed>" : *Name, arrayRefFromStringRef(*Contents),
                         Section.getAddress());
        if (auto E = Result.takeError())
            return E;
        Total.add(*Result);
    }

    if (!HasCode)
        return createStringError("no non-empty executable sections in '%s'",
                                 opts::InputFile.c_str());
    return finish(Total);
}

static Error lintInput() {
    ErrorOr<std::unique_ptr<MemoryBuffer>> Buffer =
        MemoryBuffer::getFile(opts::InputFile, /*IsText=*/false,
                              /*RequiresNullTerminator=*/false);
    if (!Buffer)
        return createStringError(Buffer.getError(), "cannot read '%s'", opts::InputFile.c_str());
    if ((*Buffer)->getBufferSize() == 0)
        return createStringError("input '%s' is empty", opts::InputFile.c_str());

    MemoryBufferRef Ref = (*Buffer)->getMemBufferRef();
    return opts::InputFormat == InputFormat::Raw ? lintRaw(Ref) : lintELF(Ref);
}

int main(int argc, char **argv) {
    InitLLVM TheInitLLVM(argc, argv);

    cl::HideUnrelatedOptions({&opts::LoongLintCategory, &getColorCategory()});
    cl::SetVersionPrinter(printVersion);
    if (!cl::ParseCommandLineOptions(argc, argv, "Lint LoongArch machine code\n", &errs()))
        return 2;
    if (!validateOptions())
        return 2;
    if (opts::ListChecks)
        return 0;

    LLVMInitializeLoongArchTargetInfo();
    LLVMInitializeLoongArchTargetMC();
    LLVMInitializeLoongArchDisassembler();

    if (Error Err = lintInput()) {
        printError(toString(std::move(Err)));
        return 2;
    }
    return 0;
}
