// SPDX-License-Identifier: GPL-3.0-or-later

#include "loonglint/RISCV/RISCVSpec.hpp"

#include "loonglint/Rules/RISCV/NopRule.hpp"

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
