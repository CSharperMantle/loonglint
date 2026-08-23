// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LOONGLINT_DECODER_HPP
#define LOONGLINT_DECODER_HPP

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCDisassembler/MCDisassembler.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/Error.h"

#include <cstdint>
#include <memory>

namespace loonglint {

enum class Architecture { Unspecified, LoongArch32, LoongArch64 };

class Decoder {
  public:
    struct Gap {
        uint64_t Begin;
        uint64_t End;
    };

    struct Result {
        uint64_t DecodedInstructions = 0;
        uint64_t SkippedWords = 0;
        uint64_t TrailingBytes = 0;
        llvm::SmallVector<Gap, 0> Gaps;
    };

    static llvm::Expected<Decoder> create(Architecture Arch);

    llvm::Expected<Result> decode(llvm::ArrayRef<uint8_t> Bytes, uint64_t Address) const;
    void setABIVersion(unsigned Version);

  private:
    Decoder() = default;

    std::unique_ptr<llvm::MCRegisterInfo> MRI;
    std::unique_ptr<llvm::MCAsmInfo> MAI;
    std::unique_ptr<llvm::MCSubtargetInfo> MSTI;
    std::unique_ptr<llvm::MCContext> Ctx;
    std::unique_ptr<llvm::MCDisassembler> Disasm;
};

} // namespace loonglint

#endif
