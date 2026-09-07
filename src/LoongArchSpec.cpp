// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/LoongArchSpec.hpp"

#include "loonglint/Rules/AddiPairRule.hpp"
#include "loonglint/Rules/AddressLoadRule.hpp"
#include "loonglint/Rules/AndNotRule.hpp"
#include "loonglint/Rules/BitCountRule.hpp"
#include "loonglint/Rules/BitExtractRule.hpp"
#include "loonglint/Rules/BitReverseRule.hpp"
#include "loonglint/Rules/BranchToNextRule.hpp"
#include "loonglint/Rules/ByteReverseRule.hpp"
#include "loonglint/Rules/DegenerateBranchRule.hpp"
#include "loonglint/Rules/IndexedLoadRule.hpp"
#include "loonglint/Rules/LoadExtendRule.hpp"
#include "loonglint/Rules/LoadZeroExtendRule.hpp"
#include "loonglint/Rules/MulhSextRule.hpp"
#include "loonglint/Rules/NopLA32Rule.hpp"
#include "loonglint/Rules/NopLA64Rule.hpp"
#include "loonglint/Rules/NopRule.hpp"
#include "loonglint/Rules/NotOrRule.hpp"
#include "loonglint/Rules/OrNotRule.hpp"
#include "loonglint/Rules/RotateCombineRule.hpp"
#include "loonglint/Rules/ShiftAddAlslDRule.hpp"
#include "loonglint/Rules/ShiftChainRule.hpp"
#include "loonglint/Rules/ShiftDoubleRule.hpp"
#include "loonglint/Rules/ShiftMaskRule.hpp"
#include "loonglint/Rules/UnsignedLoadPickRule.hpp"
#include "loonglint/Rules/ZeroExtendRule.hpp"

#include "llvm/BinaryFormat/ELF.h"

#include <memory>

using namespace llvm;

namespace loonglint {

LoongArchSpec::LoongArchSpec(bool Is64) : Is64(Is64) {}

StringRef LoongArchSpec::getName() const {
    return "loongarch";
}

unsigned LoongArchSpec::getCellSize() const {
    return 4;
}

Triple LoongArchSpec::getTriple() const {
    return Triple(Is64 ? "loongarch64" : "loongarch32");
}

uint16_t LoongArchSpec::getELFMachine() const {
    return ELF::EM_LOONGARCH;
}

MCSubtarget LoongArchSpec::getMCSubtarget() const {
    return {};
}

SmallVector<std::unique_ptr<Rule>, 0> LoongArchSpec::createRules() const {
    SmallVector<std::unique_ptr<Rule>, 0> Rules;
    Rules.emplace_back(std::make_unique<NopRule>());
    Rules.emplace_back(std::make_unique<NopLA32Rule>(*this));
    Rules.emplace_back(std::make_unique<NopLA64Rule>(*this));
    Rules.emplace_back(std::make_unique<BitExtractRule>());
    Rules.emplace_back(std::make_unique<ZeroExtendRule>());
    Rules.emplace_back(std::make_unique<BitReverseRule>(*this));
    Rules.emplace_back(std::make_unique<ByteReverseRule>(*this));
    Rules.emplace_back(std::make_unique<MulhSextRule>(*this));
    Rules.emplace_back(std::make_unique<ShiftChainRule>());
    Rules.emplace_back(std::make_unique<ShiftDoubleRule>());
    Rules.emplace_back(std::make_unique<RotateCombineRule>());
    Rules.emplace_back(std::make_unique<ShiftMaskRule>(*this));
    Rules.emplace_back(std::make_unique<AndNotRule>());
    Rules.emplace_back(std::make_unique<OrNotRule>());
    Rules.emplace_back(std::make_unique<NotOrRule>());
    Rules.emplace_back(std::make_unique<BitCountRule>());
    Rules.emplace_back(std::make_unique<BranchToNextRule>());
    Rules.emplace_back(std::make_unique<DegenerateBranchRule>());
    Rules.emplace_back(std::make_unique<ShiftAddAlslDRule>(*this));
    Rules.emplace_back(std::make_unique<AddiPairRule>());
    Rules.emplace_back(std::make_unique<AddressLoadRule>(*this));
    Rules.emplace_back(std::make_unique<LoadExtendRule>(*this));
    Rules.emplace_back(std::make_unique<IndexedLoadRule>(*this));
    Rules.emplace_back(std::make_unique<LoadZeroExtendRule>(*this));
    Rules.emplace_back(std::make_unique<UnsignedLoadPickRule>(*this));
    return Rules;
}

std::unique_ptr<ArchSpec> makeLoongArchSpec(bool Is64) {
    return std::make_unique<LoongArchSpec>(Is64);
}

} // namespace loonglint
