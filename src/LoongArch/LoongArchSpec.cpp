// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/LoongArch/LoongArchSpec.hpp"

#include "loonglint/Rules/LoongArch/AddiPairRule.hpp"
#include "loonglint/Rules/LoongArch/AddressLoadRule.hpp"
#include "loonglint/Rules/LoongArch/AndNotRule.hpp"
#include "loonglint/Rules/LoongArch/BitCountRule.hpp"
#include "loonglint/Rules/LoongArch/BitExtractRule.hpp"
#include "loonglint/Rules/LoongArch/BitReverseRule.hpp"
#include "loonglint/Rules/LoongArch/BranchToNextRule.hpp"
#include "loonglint/Rules/LoongArch/ByteReverseRule.hpp"
#include "loonglint/Rules/LoongArch/DegenerateBranchRule.hpp"
#include "loonglint/Rules/LoongArch/IndexedLoadRule.hpp"
#include "loonglint/Rules/LoongArch/LoadExtendRule.hpp"
#include "loonglint/Rules/LoongArch/LoadZeroExtendRule.hpp"
#include "loonglint/Rules/LoongArch/MulhSextRule.hpp"
#include "loonglint/Rules/LoongArch/NopLA32Rule.hpp"
#include "loonglint/Rules/LoongArch/NopLA64Rule.hpp"
#include "loonglint/Rules/LoongArch/NopRule.hpp"
#include "loonglint/Rules/LoongArch/NotOrRule.hpp"
#include "loonglint/Rules/LoongArch/OrNotRule.hpp"
#include "loonglint/Rules/LoongArch/RotateCombineRule.hpp"
#include "loonglint/Rules/LoongArch/ShiftAddAlslDRule.hpp"
#include "loonglint/Rules/LoongArch/ShiftChainRule.hpp"
#include "loonglint/Rules/LoongArch/ShiftDoubleRule.hpp"
#include "loonglint/Rules/LoongArch/ShiftMaskRule.hpp"
#include "loonglint/Rules/LoongArch/UnsignedLoadPickRule.hpp"
#include "loonglint/Rules/LoongArch/ZeroExtendRule.hpp"

#include "llvm/BinaryFormat/ELF.h"

#include <memory>
#include <type_traits>
#include <variant>

using namespace llvm;

namespace loonglint {

namespace LoongArch {

namespace {

template <class... T> constexpr bool AlwaysFalse = false;

} // namespace

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
    Rules.emplace_back(std::make_unique<LoongArch::NopRule>());
    Rules.emplace_back(std::make_unique<LoongArch::NopLA32Rule>(*this));
    Rules.emplace_back(std::make_unique<LoongArch::NopLA64Rule>(*this));
    Rules.emplace_back(std::make_unique<LoongArch::BitExtractRule>());
    Rules.emplace_back(std::make_unique<LoongArch::ZeroExtendRule>());
    Rules.emplace_back(std::make_unique<LoongArch::BitReverseRule>(*this));
    Rules.emplace_back(std::make_unique<LoongArch::ByteReverseRule>(*this));
    Rules.emplace_back(std::make_unique<LoongArch::MulhSextRule>(*this));
    Rules.emplace_back(std::make_unique<LoongArch::ShiftChainRule>());
    Rules.emplace_back(std::make_unique<LoongArch::ShiftDoubleRule>());
    Rules.emplace_back(std::make_unique<LoongArch::RotateCombineRule>());
    Rules.emplace_back(std::make_unique<LoongArch::ShiftMaskRule>(*this));
    Rules.emplace_back(std::make_unique<LoongArch::AndNotRule>());
    Rules.emplace_back(std::make_unique<LoongArch::OrNotRule>());
    Rules.emplace_back(std::make_unique<LoongArch::NotOrRule>());
    Rules.emplace_back(std::make_unique<LoongArch::BitCountRule>());
    Rules.emplace_back(std::make_unique<LoongArch::BranchToNextRule>());
    Rules.emplace_back(std::make_unique<LoongArch::DegenerateBranchRule>());
    Rules.emplace_back(std::make_unique<LoongArch::ShiftAddAlslDRule>(*this));
    Rules.emplace_back(std::make_unique<LoongArch::AddiPairRule>());
    Rules.emplace_back(std::make_unique<LoongArch::AddressLoadRule>(*this));
    Rules.emplace_back(std::make_unique<LoongArch::LoadExtendRule>(*this));
    Rules.emplace_back(std::make_unique<LoongArch::IndexedLoadRule>(*this));
    Rules.emplace_back(std::make_unique<LoongArch::LoadZeroExtendRule>(*this));
    Rules.emplace_back(std::make_unique<LoongArch::UnsignedLoadPickRule>(*this));
    return Rules;
}

void queryArchSpec(std::unique_ptr<ArchSpec> &AS, const ArchQuery &AQ) {
    std::visit(
        [&](auto &&Data) {
            using T = std::decay_t<decltype(Data)>;
            if constexpr (std::is_same_v<T, ArchQuery::Raw>) {
                if (Data.Name == "loongarch64")
                    AS = std::make_unique<LoongArchSpec>(true);
                else if (Data.Name == "loongarch32")
                    AS = std::make_unique<LoongArchSpec>(false);
            } else if constexpr (std::is_same_v<T, ArchQuery::ELF>) {
                if (Data.EMachine == ELF::EM_LOONGARCH)
                    AS = std::make_unique<LoongArchSpec>(Data.Is64);
            } else
                static_assert(AlwaysFalse<T>, "non-exhaustive visitor");
        },
        AQ.Data);
}

} // namespace LoongArch

} // namespace loonglint
