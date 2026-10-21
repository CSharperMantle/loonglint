# SPDX-License-Identifier: GPL-3.0-or-later
# pyright: reportUnusedCallResult=false

import struct
from typing import NamedTuple, final

from construct import Bytes, Int8ul, Int16ul, Int32ul, Int64ul, Struct
from elftools.elf.constants import E_FLAGS, SH_FLAGS
from elftools.elf.enums import (
    ENUM_ATTR_TAG_RISCV,
    ENUM_E_MACHINE,
    ENUM_E_TYPE,
    ENUM_E_VERSION,
    ENUM_EI_CLASS,
    ENUM_EI_DATA,
    ENUM_SH_TYPE_BASE,
    ENUM_SH_TYPE_RISCV,
)

EI_MAG = b"\x7fELF"


def _align4(n: int) -> int:
    return (n + 3) & ~3


@final
class ArchConfig(NamedTuple):
    """The ELF identity of the output: e_machine, e_flags, and an optional
    RISC-V ISA string embedded as a synthesized .riscv.attributes section
    (None for LoongArch, whose extension channel is e_flags alone).
    """

    machine: int
    flags: int
    isa: str | None = None


LOONGARCH64 = ArchConfig(
    ENUM_E_MACHINE["EM_LOONGARCH"],
    E_FLAGS.EF_LOONGARCH_ABI_DOUBLE_FLOAT | E_FLAGS.EF_LOONGARCH_OBJABI_V1,
)


@final
class _StrTab:
    """A byte buffer for a string table (shstrtab or strtab)."""

    def __init__(self) -> None:
        self._buf = bytearray(b"\x00")
        self._offs: dict[str, int] = {}

    def add(self, s: str) -> int:
        if s in self._offs:
            return self._offs[s]
        off = len(self._buf)
        self._buf.extend(s.encode() + b"\x00")
        self._offs[s] = off
        return off

    def bytes(self) -> bytes:
        return bytes(self._buf)


# Elf64_Ehdr (little-endian), field order per the generic ABI.
Elf64_Ehdr = Struct(
    "e_ident" / Bytes(16),
    "e_type" / Int16ul,
    "e_machine" / Int16ul,
    "e_version" / Int32ul,
    "e_entry" / Int64ul,
    "e_phoff" / Int64ul,
    "e_shoff" / Int64ul,
    "e_flags" / Int32ul,
    "e_ehsize" / Int16ul,
    "e_phentsize" / Int16ul,
    "e_phnum" / Int16ul,
    "e_shentsize" / Int16ul,
    "e_shnum" / Int16ul,
    "e_shstrndx" / Int16ul,
)

# Elf64_Shdr (little-endian), field order per the generic ABI.
Elf64_Shdr = Struct(
    "sh_name" / Int32ul,
    "sh_type" / Int32ul,
    "sh_flags" / Int64ul,
    "sh_addr" / Int64ul,
    "sh_offset" / Int64ul,
    "sh_size" / Int64ul,
    "sh_link" / Int32ul,
    "sh_info" / Int32ul,
    "sh_addralign" / Int64ul,
    "sh_entsize" / Int64ul,
)

# Elf64_Sym (little-endian), field order per the generic ABI: st_name(4),
# st_info(1), st_other(1), st_shndx(2), st_value(8), st_size(8) = 24 bytes.
Elf64_Sym = Struct(
    "st_name" / Int32ul,
    "st_info" / Int8ul,
    "st_other" / Int8ul,
    "st_shndx" / Int16ul,
    "st_value" / Int64ul,
    "st_size" / Int64ul,
)

# st_info = (bind << 4) | (type & 0xf). STB_LOCAL=0, STT_FUNC=2.
STB_LOCAL = 0
STT_FUNC = 2

# Fixed-size header/entry widths, derived from the schemas above.
EH_SIZE = Elf64_Ehdr.sizeof()
SHDR_SIZE = Elf64_Shdr.sizeof()


def riscv_attributes(isa: str) -> bytes:
    """Serialize a GNU-attributes section carrying Tag_RISCV_arch == |isa|.

    Layout:
    - 'A'
    - u32 SectionLength counting everything after 'A' itself
    - vendor "riscv",
    - one Tag_File subsection whose Size covers the tag byte, the size field, and the attribute list
    - the list is a ULEB128 attribute tag followed by the ISA string.
    """
    attr_list = bytes([ENUM_ATTR_TAG_RISCV["TAG_ARCH"]]) + isa.encode() + b"\0"
    subsection = (
        bytes([ENUM_ATTR_TAG_RISCV["TAG_FILE"]]) + struct.pack("<I", 5 + len(attr_list)) + attr_list
    )
    body = b"riscv\0" + subsection
    return b"A" + struct.pack("<I", len(body) + 4) + body


def _shdr(
    sh_name: int = 0,
    sh_type: int = ENUM_SH_TYPE_BASE["SHT_NULL"],
    sh_flags: int = 0,
    sh_addr: int = 0,
    sh_offset: int = 0,
    sh_size: int = 0,
    sh_link: int = 0,
    sh_info: int = 0,
    sh_addralign: int = 0,
    sh_entsize: int = 0,
) -> bytes:
    return Elf64_Shdr.build(
        {
            "sh_name": sh_name,
            "sh_type": sh_type,
            "sh_flags": sh_flags,
            "sh_addr": sh_addr,
            "sh_offset": sh_offset,
            "sh_size": sh_size,
            "sh_link": sh_link,
            "sh_info": sh_info,
            "sh_addralign": sh_addralign,
            "sh_entsize": sh_entsize,
        }
    )


def _ehdr_ident() -> bytes:
    """The 16-byte e_ident, from canonical enum values."""
    return (
        EI_MAG
        + bytes(
            (
                ENUM_EI_CLASS["ELFCLASS64"],
                ENUM_EI_DATA["ELFDATA2LSB"],
                ENUM_E_VERSION["EV_CURRENT"],
                0,
                0,
            )
        )
        + b"\x00" * 7
    )


def write_elf(
    sections: list[tuple[str, int, bytes]], path: str, arch: ArchConfig = LOONGARCH64
) -> None:
    """Write a 64-bit little-endian ET_EXEC ELF of the given architecture
    with one unnamed section per (name, addr, code), plus
    symtab/strtab/shstrtab.

    Each section's sh_addr is the original JIT address, so finding
    addresses cross-reference back into the jitdump. Section names stay
    empty: the symbol table is the reporting channel, carrying the raw
    jitdump record name as an STT_FUNC symbol at the record's original
    address with the record's code size.
    """
    payload = bytearray()
    sec_offs: list[int] = []
    for _name, _addr, code in sections:
        sec_offs.append(EH_SIZE + len(payload))
        payload += code
        payload += b"\x00" * (_align4(len(payload)) - len(payload))

    shstrtab = _StrTab()
    symtab_name = shstrtab.add(".symtab")
    strtab_name = shstrtab.add(".strtab")
    shstrtab_name = shstrtab.add(".shstrtab")
    attr = riscv_attributes(arch.isa) if arch.isa else b""
    attr_name = shstrtab.add(".riscv.attributes") if attr else None

    strtab = _StrTab()
    syms = bytearray(
        Elf64_Sym.build(
            {"st_name": 0, "st_info": 0, "st_other": 0, "st_shndx": 0, "st_value": 0, "st_size": 0}
        )
    )
    for i, (name, addr, code) in enumerate(sections):
        st_name = strtab.add(name)
        syms += Elf64_Sym.build(
            {
                "st_name": st_name,
                "st_info": (STB_LOCAL << 4) | STT_FUNC,
                "st_other": 0,
                "st_shndx": 1 + i,
                "st_value": addr,
                "st_size": len(code),
            }
        )

    nsections = 1 + len(sections) + 3
    symtab_idx = 1 + len(sections)
    strtab_idx = symtab_idx + 1
    shstrtab_idx = strtab_idx + 1

    symtab_off = EH_SIZE + len(payload)
    strtab_off = symtab_off + len(syms)
    shstrtab_off = strtab_off + len(strtab.bytes())
    attr_off = shstrtab_off + len(shstrtab.bytes())
    shoff = _align4(attr_off + len(attr))
    nsections += 1 if attr else 0

    shdrs = bytearray(_shdr())
    for (_name, addr, code), off in zip(sections, sec_offs, strict=True):
        shdrs += _shdr(
            sh_name=0,
            sh_type=ENUM_SH_TYPE_BASE["SHT_PROGBITS"],
            sh_flags=SH_FLAGS.SHF_ALLOC | SH_FLAGS.SHF_EXECINSTR,
            sh_addr=addr,
            sh_offset=off,
            sh_size=len(code),
            sh_addralign=4,
        )
    shdrs += _shdr(
        sh_name=symtab_name,
        sh_type=ENUM_SH_TYPE_BASE["SHT_SYMTAB"],
        sh_offset=symtab_off,
        sh_size=len(syms),
        sh_link=strtab_idx,
        sh_info=1 + len(sections),
        sh_addralign=8,
        sh_entsize=Elf64_Sym.sizeof(),
    )
    shdrs += _shdr(
        sh_name=strtab_name,
        sh_type=ENUM_SH_TYPE_BASE["SHT_STRTAB"],
        sh_offset=strtab_off,
        sh_size=len(strtab.bytes()),
        sh_addralign=1,
    )
    shdrs += _shdr(
        sh_name=shstrtab_name,
        sh_type=ENUM_SH_TYPE_BASE["SHT_STRTAB"],
        sh_offset=shstrtab_off,
        sh_size=len(shstrtab.bytes()),
        sh_addralign=1,
    )
    if attr:
        assert attr_name is not None
        shdrs += _shdr(
            sh_name=attr_name,
            sh_type=ENUM_SH_TYPE_RISCV["SHT_RISCV_ATTRIBUTES"],
            sh_offset=attr_off,
            sh_size=len(attr),
            sh_addralign=1,
        )

    ehdr = Elf64_Ehdr.build(
        {
            "e_ident": _ehdr_ident(),
            "e_type": ENUM_E_TYPE["ET_EXEC"],
            "e_machine": arch.machine,
            "e_version": ENUM_E_VERSION["EV_CURRENT"],
            "e_entry": 0,
            "e_phoff": 0,
            "e_shoff": shoff,
            "e_flags": arch.flags,
            "e_ehsize": EH_SIZE,
            "e_phentsize": 0,
            "e_phnum": 0,
            "e_shentsize": SHDR_SIZE,
            "e_shnum": nsections,
            "e_shstrndx": shstrtab_idx,
        }
    )

    with open(path, "wb") as f:
        f.write(ehdr)
        f.write(payload)
        f.write(syms)
        f.write(strtab.bytes())
        f.write(shstrtab.bytes())
        f.write(attr)
        f.write(b"\x00" * (shoff - attr_off - len(attr)))
        f.write(shdrs)
