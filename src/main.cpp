// SPDX-License-Identifier: GPL-3.0-or-later

#include "llvm/ADT/StringRef.h"
#include "llvm/Config/llvm-config.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/WithColor.h"
#include "llvm/Support/raw_ostream.h"

#include <cstdint>
#include <string>

using namespace llvm;

enum class InputFormat { Auto, Elf, Raw };
enum class Architecture { Unspecified, LoongArch32, LoongArch64 };

namespace opts {

cl::OptionCategory LoongLintCategory("loonglint options");

cl::opt<std::string> InputFile(cl::Positional, cl::desc("<input file>"), cl::Optional,
                               cl::cat(LoongLintCategory));

cl::opt<InputFormat>
    InputFormatOption("input-format", cl::desc("Input format"), cl::init(InputFormat::Auto),
                      cl::values(clEnumValN(InputFormat::Auto, "auto", "Recognize ELF input"),
                                 clEnumValN(InputFormat::Elf, "elf", "Require ELF input"),
                                 clEnumValN(InputFormat::Raw, "raw", "Treat input as raw code")),
                      cl::cat(LoongLintCategory));

cl::opt<Architecture> ArchitectureOption(
    "arch", cl::desc("Architecture for raw input"), cl::init(Architecture::Unspecified),
    cl::values(clEnumValN(Architecture::LoongArch32, "loongarch32", "LoongArch32"),
               clEnumValN(Architecture::LoongArch64, "loongarch64", "LoongArch64")),
    cl::cat(LoongLintCategory));

cl::opt<std::uint64_t> BaseAddress("base-address", cl::desc("Base address for raw input"),
                                   cl::init(0), cl::value_desc("integer"),
                                   cl::cat(LoongLintCategory));

cl::opt<bool> ListChecks("list-checks", cl::desc("List available checks"),
                         cl::cat(LoongLintCategory));

cl::opt<bool> Verbose("verbose", cl::desc("Show verbose findings"), cl::cat(LoongLintCategory));
cl::alias VerboseShort("v", cl::NotHidden, cl::desc("Alias for --verbose"), cl::aliasopt(Verbose),
                       cl::cat(LoongLintCategory));

} // namespace opts

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

    if (opts::InputFormatOption == InputFormat::Raw) {
        if (opts::ArchitectureOption == Architecture::Unspecified) {
            printError("--arch is required with --input-format=raw");
            return false;
        }
        return true;
    }

    if (opts::ArchitectureOption.getNumOccurrences() != 0) {
        printError("--arch is valid only with --input-format=raw");
        return false;
    }

    if (opts::BaseAddress.getNumOccurrences() != 0) {
        printError("--base-address is valid only with --input-format=raw");
        return false;
    }

    return true;
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

    printError("input scanning is not implemented yet");
    return 2;
}
