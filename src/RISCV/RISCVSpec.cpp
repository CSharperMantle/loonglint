// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/RISCV/RISCVSpec.hpp"

#include "loonglint/Rules/RISCV/AddiPairRule.hpp"
#include "loonglint/Rules/RISCV/AddressLoadRule.hpp"
#include "loonglint/Rules/RISCV/AndNotRule.hpp"
#include "loonglint/Rules/RISCV/BclrRule.hpp"
#include "loonglint/Rules/RISCV/BextRule.hpp"
#include "loonglint/Rules/RISCV/BinvRule.hpp"
#include "loonglint/Rules/RISCV/BitRepeatRule.hpp"
#include "loonglint/Rules/RISCV/BranchToNextRule.hpp"
#include "loonglint/Rules/RISCV/BsetRule.hpp"
#include "loonglint/Rules/RISCV/DegenerateBranchRule.hpp"
#include "loonglint/Rules/RISCV/LoadZextRule.hpp"
#include "loonglint/Rules/RISCV/LogicImmediateRule.hpp"
#include "loonglint/Rules/RISCV/NegRule.hpp"
#include "loonglint/Rules/RISCV/NopRule.hpp"
#include "loonglint/Rules/RISCV/NopZbaRule.hpp"
#include "loonglint/Rules/RISCV/NopZbbRule.hpp"
#include "loonglint/Rules/RISCV/NopZbkbRule.hpp"
#include "loonglint/Rules/RISCV/NotXorRule.hpp"
#include "loonglint/Rules/RISCV/OrNotRule.hpp"
#include "loonglint/Rules/RISCV/PCBranchRule.hpp"
#include "loonglint/Rules/RISCV/RotateCombineRule.hpp"
#include "loonglint/Rules/RISCV/SextElimRule.hpp"
#include "loonglint/Rules/RISCV/SextFormRule.hpp"
#include "loonglint/Rules/RISCV/SextWElimRule.hpp"
#include "loonglint/Rules/RISCV/SextWFormRule.hpp"
#include "loonglint/Rules/RISCV/ShiftAddRule.hpp"
#include "loonglint/Rules/RISCV/ShiftChainRule.hpp"
#include "loonglint/Rules/RISCV/ShiftMaskRule.hpp"
#include "loonglint/Rules/RISCV/ZextHElimRule.hpp"
#include "loonglint/Rules/RISCV/ZextHFormRule.hpp"
#include "loonglint/Rules/RISCV/ZextWElimRule.hpp"
#include "loonglint/Rules/RISCV/ZextWFoldRule.hpp"
#include "loonglint/Rules/RISCV/ZextWFormRule.hpp"

#include "llvm/ADT/SmallVector.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/Support/Error.h"
#include "llvm/TargetParser/RISCVISAInfo.h"
#include "llvm/TargetParser/SubtargetFeature.h"
#include "llvm/TargetParser/Triple.h"

#include <memory>
#include <utility>

using namespace llvm;

namespace loonglint {

namespace RISCV {

namespace {

template <class... T> constexpr bool AlwaysFalse = false;

} // namespace

RISCVSpec::RISCVSpec(unsigned XLen, std::string FeaturesString)
    : XLen(XLen), FeaturesString(std::move(FeaturesString)) {}

std::unique_ptr<RISCVSpec> RISCVSpec::createFromArchString(StringRef Arch) {
    auto Info = RISCVISAInfo::parseArchString(Arch, /*EnableExperimentalExtension=*/false);
    if (!Info) {
        consumeError(Info.takeError());
        return nullptr;
    }

    SubtargetFeatures Features;
    Features.AddFeature("64bit", (*Info)->getXLen() == 64);
    Features.addFeaturesVector((*Info)->toFeatures());

    return std::make_unique<RISCVSpec>((*Info)->getXLen(), Features.getString());
}

StringRef RISCVSpec::getName() const {
    return "riscv";
}

bool RISCVSpec::hasExtension(StringRef Ext) const {
    SmallVector<StringRef, 32> Features;
    StringRef(FeaturesString).split(Features, ',');
    for (StringRef Feature : Features)
        if (Feature.size() == Ext.size() + 1 && Feature.front() == '+' &&
            Feature.drop_front() == Ext)
            return true;
    return false;
}

unsigned RISCVSpec::getCellSize() const {
    return 2;
}

Triple RISCVSpec::getTriple() const {
    return Triple(isRV64() ? "riscv64" : "riscv32");
}

uint16_t RISCVSpec::getELFMachine() const {
    return ELF::EM_RISCV;
}

MCSubtarget RISCVSpec::getMCSubtarget() const {
    return {/*CPU=*/"", FeaturesString};
}

SmallVector<std::unique_ptr<Rule>, 0> RISCVSpec::createRules() const {
    SmallVector<std::unique_ptr<Rule>, 0> Rules;
    Rules.emplace_back(std::make_unique<RISCV::NopRule>());
    Rules.emplace_back(std::make_unique<RISCV::NopZbaRule>(*this));
    Rules.emplace_back(std::make_unique<RISCV::NopZbbRule>(*this));
    Rules.emplace_back(std::make_unique<RISCV::NopZbkbRule>(*this));
    Rules.emplace_back(std::make_unique<RISCV::BitRepeatRule>(*this));
    Rules.emplace_back(std::make_unique<RISCV::SextWFormRule>(*this));
    Rules.emplace_back(std::make_unique<RISCV::SextWElimRule>(*this));
    Rules.emplace_back(std::make_unique<RISCV::SextFormRule>(*this));
    Rules.emplace_back(std::make_unique<RISCV::SextElimRule>(*this));
    Rules.emplace_back(std::make_unique<RISCV::ZextHFormRule>(*this));
    Rules.emplace_back(std::make_unique<RISCV::ZextHElimRule>(*this));
    Rules.emplace_back(std::make_unique<RISCV::ZextWFormRule>(*this));
    Rules.emplace_back(std::make_unique<RISCV::ZextWElimRule>(*this));
    Rules.emplace_back(std::make_unique<RISCV::ZextWFoldRule>(*this));
    Rules.emplace_back(std::make_unique<RISCV::ShiftChainRule>(*this));
    Rules.emplace_back(std::make_unique<RISCV::ShiftMaskRule>(*this));
    Rules.emplace_back(std::make_unique<RISCV::ShiftAddRule>(*this));
    Rules.emplace_back(std::make_unique<RISCV::RotateCombineRule>(*this));
    Rules.emplace_back(std::make_unique<RISCV::AndNotRule>(*this));
    Rules.emplace_back(std::make_unique<RISCV::OrNotRule>(*this));
    Rules.emplace_back(std::make_unique<RISCV::NotXorRule>(*this));
    Rules.emplace_back(std::make_unique<RISCV::BextRule>(*this));
    Rules.emplace_back(std::make_unique<RISCV::BclrRule>(*this));
    Rules.emplace_back(std::make_unique<RISCV::BsetRule>(*this));
    Rules.emplace_back(std::make_unique<RISCV::BinvRule>(*this));
    Rules.emplace_back(std::make_unique<RISCV::LogicImmediateRule>());
    Rules.emplace_back(std::make_unique<RISCV::AddiPairRule>());
    Rules.emplace_back(std::make_unique<RISCV::NegRule>());
    Rules.emplace_back(std::make_unique<RISCV::AddressLoadRule>(*this));
    Rules.emplace_back(std::make_unique<RISCV::LoadZextRule>());
    Rules.emplace_back(std::make_unique<RISCV::BranchToNextRule>());
    Rules.emplace_back(std::make_unique<RISCV::DegenerateBranchRule>());
    Rules.emplace_back(std::make_unique<RISCV::PCBranchRule>());
    return Rules;
}

void queryArchSpec(std::unique_ptr<ArchSpec> &AS, const ArchQuery &AQ) {
    std::visit(
        [&](auto &&Data) {
            using T = std::decay_t<decltype(Data)>;
            if constexpr (std::is_same_v<T, ArchQuery::Raw>) {
                AS = RISCVSpec::createFromArchString(Data.Name);
            } else if constexpr (std::is_same_v<T, ArchQuery::ELF>) {
                if (Data.EMachine == ELF::EM_RISCV)
                    AS =
                        std::make_unique<RISCVSpec>(Data.Is64 ? 64 : 32, Data.Features.getString());
            } else
                static_assert(AlwaysFalse<T>, "non-exhaustive visitor");
        },
        AQ.Data);
}

} // namespace RISCV

} // namespace loonglint
