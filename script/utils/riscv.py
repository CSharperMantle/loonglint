# SPDX-License-Identifier: GPL-3.0-or-later
# pyright: reportUnusedCallResult=false

from elftools.elf.constants import E_FLAGS
from elftools.elf.enums import ENUM_E_MACHINE

from .elf import ArchConfig

# Extension versions mirror the RISCVExtension<Major, Minor> records.
# https://github.com/llvm/llvm-project/blob/fa01521e37e1b2db4edbf6a41687a00153505a37/llvm/lib/Target/RISCV/RISCVFeatures.td
# i:L74 e:L78 m:L224 a:L251 f:L308 d:L316 c:L418 zicsr:L134 zifencei:L152 zba:L496 zbb:L504 zbs:L513 zmmul:L217
RISCV_EXT_VERSIONS = {
    "i": "2p1",
    "e": "2p0",
    "m": "2p0",
    "a": "2p1",
    "f": "2p2",
    "d": "2p2",
    "c": "2p0",
    "zicsr": "2p0",
    "zifencei": "2p0",
    "zmmul": "1p0",
    "zba": "1p0",
    "zbb": "1p0",
    "zbc": "1p0",
    "zbs": "1p0",
}


def _riscv_token_is_versioned(token: str) -> bool:
    p = token.rfind("p")
    return p > 0 and token[:p][-1:].isdigit() and token[p + 1 :].isdigit()


def _riscv_base_letters(token: str) -> str:
    """Expand the base token: 'g' to 'imafd' (RISCVGImplications; no 'c'),
    versions stripped, order-preserving dedupe.
    """
    letters: list[str] = []
    for ch in token:
        if ch.isdigit():
            break
        letters += "imafd" if ch == "g" else ch
    return "".join(dict.fromkeys(letters))


def normalize_riscv_isa(isa: str) -> str:
    """Turn a friendly ISA string (rv64gc_zbb) into the versioned form the
    strict normalized-arch parser requires (rv64i2p1_m2p0_..._zbb1p0).
    'g' expands to the 'imafd' single-letter set; versioned tokens pass
    through; unknown extensions default to version 1p0.
    """
    tokens: list[tuple[str, str]] = []
    for i, token in enumerate(isa[4:].split("_")):
        if i == 0 and not _riscv_token_is_versioned(token):
            tokens += [(ch, RISCV_EXT_VERSIONS.get(ch, "1p0")) for ch in _riscv_base_letters(token)]
        elif _riscv_token_is_versioned(token):
            p = token.rfind("p")
            tokens.append((token[: p + 1], token[p + 1 :]))
        else:
            tokens.append((token, RISCV_EXT_VERSIONS.get(token, "1p0")))
    return "rv64" + "_".join(name + version for name, version in tokens)


def riscv_config(isa: str) -> ArchConfig:
    normalized = normalize_riscv_isa(isa)
    letters = _riscv_base_letters(isa[4:].split("_", 1)[0])
    flags = 0
    if "c" in letters:
        flags |= E_FLAGS.EF_RISCV_RVC
    if "d" in letters:
        flags |= E_FLAGS.EF_RISCV_FLOAT_ABI_DOUBLE
    elif "f" in letters:
        flags |= E_FLAGS.EF_RISCV_FLOAT_ABI_SINGLE
    return ArchConfig(ENUM_E_MACHINE["EM_RISCV"], flags, normalized)
