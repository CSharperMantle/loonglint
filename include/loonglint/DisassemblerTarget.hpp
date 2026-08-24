// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LOONGLINT_DISASSEMBLERTARGET_HPP
#define LOONGLINT_DISASSEMBLERTARGET_HPP

#include "llvm/ADT/ArrayRef.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCDisassembler/MCDisassembler.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstPrinter.h"
#include "llvm/MC/MCInstrAnalysis.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/raw_ostream.h"

#include <cstdint>
#include <memory>
#include <optional>

namespace loonglint {

enum class Architecture { LoongArch32, LoongArch64 };

struct Instruction {
    uint64_t Address;
    llvm::MCInst Inst;
};

class ScannedRegion;

class DisassemblerTarget {
  public:
    static llvm::Expected<DisassemblerTarget> create(Architecture TheArchitecture);

    void setABIVersion(unsigned Version);
    void printInst(const llvm::MCInst &MI, uint64_t Address, llvm::raw_ostream &Output) const;

  private:
    friend class ScannedRegion;

    explicit DisassemblerTarget(Architecture Arch) : Arch(Arch) {}

    std::optional<llvm::MCInst> decodeInst(llvm::ArrayRef<uint8_t> Word, uint64_t Address) const;

    Architecture Arch;
    std::unique_ptr<llvm::MCRegisterInfo> MRI;
    std::unique_ptr<llvm::MCInstrInfo> MII;
    std::unique_ptr<llvm::MCAsmInfo> MAI;
    std::unique_ptr<llvm::MCSubtargetInfo> MSTI;
    std::unique_ptr<llvm::MCContext> Ctx;
    std::unique_ptr<llvm::MCInstrAnalysis> MIA;
    std::unique_ptr<llvm::MCInstPrinter> Printer;
    std::unique_ptr<llvm::MCDisassembler> Disasm;
};

} // namespace loonglint

#endif
