// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/RISCV/RISCVSpec.hpp"

#include "loonglint/Rules/RISCV/AddiPairRule.hpp"
#include "loonglint/Rules/RISCV/AddressLoadRule.hpp"
#include "loonglint/Rules/RISCV/BranchToNextRule.hpp"
#include "loonglint/Rules/RISCV/DegenerateBranchRule.hpp"
#include "loonglint/Rules/RISCV/LoadExtendRule.hpp"
#include "loonglint/Rules/RISCV/LogicImmediateRule.hpp"
#include "loonglint/Rules/RISCV/NegRule.hpp"
#include "loonglint/Rules/RISCV/NopRule.hpp"
#include "loonglint/Rules/RISCV/PCBranchRule.hpp"
#include "loonglint/Rules/RISCV/SextWRule.hpp"
#include "loonglint/Rules/RISCV/ShiftChainRule.hpp"
#include "loonglint/Rules/RISCV/ShiftMaskRule.hpp"
#include "loonglint/Rules/RISCV/ZbaNopRule.hpp"
#include "loonglint/Rules/RISCV/ZbaShAddRule.hpp"
#include "loonglint/Rules/RISCV/ZbaZextWRule.hpp"
#include "loonglint/Rules/RISCV/ZbbAndnRule.hpp"
#include "loonglint/Rules/RISCV/ZbbNopRule.hpp"
#include "loonglint/Rules/RISCV/ZbbRotateRule.hpp"
#include "loonglint/Rules/RISCV/ZbbSextRule.hpp"
#include "loonglint/Rules/RISCV/ZbbZextHRule.hpp"
#include "loonglint/Rules/RISCV/ZbsBclrRule.hpp"
#include "loonglint/Rules/RISCV/ZbsBextRule.hpp"
#include "loonglint/Rules/RISCV/ZbsBsetRule.hpp"
#include "loonglint/Rules/RISCV/ZbsNopRule.hpp"

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
    Rules.emplace_back(std::make_unique<RISCV::ZbaNopRule>(*this));
    Rules.emplace_back(std::make_unique<RISCV::ZbbNopRule>(*this));
    Rules.emplace_back(std::make_unique<RISCV::ZbsNopRule>(*this));
    Rules.emplace_back(std::make_unique<RISCV::SextWRule>(*this));
    Rules.emplace_back(std::make_unique<RISCV::ZbaZextWRule>(*this));
    Rules.emplace_back(std::make_unique<RISCV::ZbbSextRule>(*this));
    Rules.emplace_back(std::make_unique<RISCV::ZbbZextHRule>(*this));
    Rules.emplace_back(std::make_unique<RISCV::AddiPairRule>());
    Rules.emplace_back(std::make_unique<RISCV::LogicImmediateRule>());
    Rules.emplace_back(std::make_unique<RISCV::ShiftChainRule>(*this));
    Rules.emplace_back(std::make_unique<RISCV::ShiftMaskRule>(*this));
    Rules.emplace_back(std::make_unique<RISCV::NegRule>());
    Rules.emplace_back(std::make_unique<RISCV::ZbaShAddRule>(*this));
    Rules.emplace_back(std::make_unique<RISCV::ZbbAndnRule>(*this));
    Rules.emplace_back(std::make_unique<RISCV::ZbbRotateRule>(*this));
    Rules.emplace_back(std::make_unique<RISCV::ZbsBextRule>(*this));
    Rules.emplace_back(std::make_unique<RISCV::ZbsBsetRule>(*this));
    Rules.emplace_back(std::make_unique<RISCV::ZbsBclrRule>(*this));
    Rules.emplace_back(std::make_unique<RISCV::BranchToNextRule>());
    Rules.emplace_back(std::make_unique<RISCV::DegenerateBranchRule>());
    Rules.emplace_back(std::make_unique<RISCV::PCBranchRule>());
    Rules.emplace_back(std::make_unique<RISCV::AddressLoadRule>(*this));
    Rules.emplace_back(std::make_unique<RISCV::LoadExtendRule>());
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
