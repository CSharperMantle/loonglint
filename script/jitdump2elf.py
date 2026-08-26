#!/usr/bin/env python3

# SPDX-License-Identifier: GPL-3.0-or-later
# pyright: reportUnusedCallResult=false

import re
import struct
from collections.abc import Iterable, Iterator, Sequence
from pathlib import Path
from typing import Annotated, NamedTuple, final

import typer
from construct import Bytes, Int8ul, Int16ul, Int32ul, Int64ul, Struct
from elftools.elf.constants import SH_FLAGS
from elftools.elf.enums import (
    ENUM_E_MACHINE,
    ENUM_E_TYPE,
    ENUM_E_VERSION,
    ENUM_EI_CLASS,
    ENUM_EI_DATA,
    ENUM_SH_TYPE_BASE,
)

EI_MAG = b"\x7fELF"

# https://searchfox.org/firefox-main/rev/600bd2128b2ba435b9698ec8b61394aa27c7c93f/js/src/jit/PerfSpewer.cpp#199
JITDUMP_MAGIC = 0x4A695444

# https://searchfox.org/firefox-main/rev/600bd2128b2ba435b9698ec8b61394aa27c7c93f/js/src/jit/Jitdump.h#19
JIT_CODE_LOAD = 0

SANITIZE_RE = re.compile(r"[^A-Za-z0-9_.@:<>/-]")


@final
class JitDumpHeader(NamedTuple):
    """C++ struct `JitDumpHeader`.

    https://searchfox.org/firefox-main/rev/600bd2128b2ba435b9698ec8b61394aa27c7c93f/js/src/jit/Jitdump.h#26-36
    """

    magic: int
    version: int
    total_size: int
    elf_mach: int
    pad1: int
    pid: int
    timestamp: int
    flags: int

    FMT = "<6I2Q"

    @classmethod
    def parse(cls, data: bytes, offset: int = 0) -> JitDumpHeader:
        return cls._make(struct.unpack_from(cls.FMT, data, offset))


@final
class JitDumpRecordHeader(NamedTuple):
    """C++ struct `JitDumpRecordHeader`.

    https://searchfox.org/firefox-main/rev/600bd2128b2ba435b9698ec8b61394aa27c7c93f/js/src/jit/Jitdump.h#38-43
    """

    id: int
    total_size: int
    timestamp: int

    FMT = "<2IQ"
    SIZE = struct.calcsize(FMT)

    @classmethod
    def parse(cls, data: bytes, offset: int) -> JitDumpRecordHeader:
        return cls._make(struct.unpack_from(cls.FMT, data, offset))


@final
class CodeObject(NamedTuple):
    """One JIT_CODE_LOAD record: a named chunk of machine code at an address."""

    name: str
    addr: int
    code: bytes

    _FIXED_FMT = "<2I4Q"
    _FIXED_SIZE = struct.calcsize(_FIXED_FMT)

    @classmethod
    def parse_code_load(cls, data: bytes, body: int) -> CodeObject | None:
        # https://searchfox.org/firefox-main/rev/600bd2128b2ba435b9698ec8b61394aa27c7c93f/js/src/jit/Jitdump.h#49-55
        _pid, _tid, _vma, code_addr, code_size, _code_index = struct.unpack_from(
            cls._FIXED_FMT, data, body
        )
        name_start = body + cls._FIXED_SIZE
        name_end = data.find(b"\x00", name_start)
        if name_end < 0:
            return None
        name = data[name_start:name_end].decode("utf-8", "replace")
        code = data[name_end + 1 : name_end + 1 + code_size]
        return cls(name, code_addr, code)


def parse_jitdump(data: bytes) -> list[CodeObject]:
    """Return all JIT_CODE_LOAD code objects from a jitdump byte stream."""
    header = JitDumpHeader.parse(data)
    if header.magic != JITDUMP_MAGIC:
        raise ValueError(f"bad magic 0x{header.magic:08x} (expected JiTD 0x{JITDUMP_MAGIC:08x})")

    objects: list[CodeObject] = []
    off = header.total_size
    n = len(data)
    while off + JitDumpRecordHeader.SIZE <= n:
        rh = JitDumpRecordHeader.parse(data, off)
        if rh.total_size < JitDumpRecordHeader.SIZE or off + rh.total_size > n:
            break
        if rh.id == JIT_CODE_LOAD:
            obj = CodeObject.parse_code_load(data, off + JitDumpRecordHeader.SIZE)
            if obj is not None:
                objects.append(obj)
        off += rh.total_size
    return objects


# --- tier attribution -----------------------------------------------------

_TIER_PREFIXES: Sequence[tuple[str, tuple[str, ...]]] = (
    ("Ion", ("Ion:", "IonIC:")),
    ("Baseline", ("Baseline:", "BaselineIC:", "BaselineICFallback:")),
    ("Interpreter", ("BlinterpOp:", "BaselineInterpreter", "Interpreter:")),
    ("Trampoline", ("Trampoline:", "VMWrapper:")),
)


def tier_of(name: str) -> str:
    for tier, prefixes in _TIER_PREFIXES:
        if name.startswith(prefixes):
            return tier
    return "Other"


def section_name(name: str, seen: dict[str, int]) -> str:
    """Sanitize a jitdump function name into a unique ELF section name."""
    base = SANITIZE_RE.sub("_", name)[:90] or "anonymous"
    n = seen.get(base, 0) + 1
    seen[base] = n
    return base if n == 1 else f"{base}.{n}"


# === ELF64 emission ===


def _align4(n: int) -> int:
    return (n + 3) & ~3


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


def write_elf(sections: list[tuple[str, int, bytes]], path: str) -> None:
    """Write a LoongArch ET_EXEC ELF with one PROGBITS+EXECINSTR section per
    (name, addr, code), plus symtab/strtab/shstrtab.

    Each section's sh_addr is the original JIT address, set directly in the
    Elf64_Shdr struct, so finding addresses cross-reference back into the
    jitdump.
    """
    # Section content, 4-aligned back to back after the ehdr.
    payload = bytearray()
    sec_offs: list[int] = []
    for _name, _addr, code in sections:
        sec_offs.append(EH_SIZE + len(payload))
        payload += code
        payload += b"\x00" * (_align4(len(payload)) - len(payload))

    # shstrtab holds every section's name. Each code section gets its own
    # unique name (the sanitized jitdump function name), so loonglint reports
    # the generating function directly in `file:section:addr`.
    shstrtab = _StrTab()
    sec_name_offs = [shstrtab.add(name) for name, _addr, _code in sections]
    symtab_name = shstrtab.add(".symtab")
    strtab_name = shstrtab.add(".strtab")
    shstrtab_name = shstrtab.add(".shstrtab")

    # Symbol table: index 0 null symbol, then one STT_FUNC per section.
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
                "st_shndx": 1 + i,  # section index (after the null section)
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
    shoff = _align4(shstrtab_off + len(shstrtab.bytes()))

    shdrs = bytearray(_shdr())
    for name_off, (_name, addr, code), off in zip(sec_name_offs, sections, sec_offs, strict=True):
        shdrs += _shdr(
            sh_name=name_off,
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

    ehdr = Elf64_Ehdr.build(
        {
            "e_ident": _ehdr_ident(),
            "e_type": ENUM_E_TYPE["ET_EXEC"],
            "e_machine": ENUM_E_MACHINE["EM_LOONGARCH"],
            "e_version": ENUM_E_VERSION["EV_CURRENT"],
            "e_entry": 0,
            "e_phoff": 0,
            "e_shoff": shoff,
            "e_flags": 0,
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
        f.write(b"\x00" * (shoff - shstrtab_off - len(shstrtab.bytes())))
        f.write(shdrs)


# === CLI ===


def _filter(
    objects: Iterable[CodeObject], only: Iterable[str], exclude: Iterable[str]
) -> Iterator[CodeObject]:
    for o in objects:
        if only and not any(o.name.startswith(p) for p in only):
            continue
        if exclude and any(o.name.startswith(p) for p in exclude):
            continue
        yield o


app = typer.Typer(
    add_completion=False,
    help="Convert SpiderMonkey JitDump files to an ELF for LoongLint consumption",
)


@app.command()
def main(
    dump: Annotated[
        Path,
        typer.Argument(
            exists=True,
            file_okay=True,
            dir_okay=False,
            help="Path to input jit-*.dump file",
        ),
    ],
    output: Annotated[
        Path,
        typer.Option(
            "-o",
            "--output",
            default_factory=lambda: Path("smjit.elf"),
            show_default="smjit.elf",
            file_okay=True,
            dir_okay=False,
            help="Path to output file",
        ),
    ],
    only: Annotated[
        list[str] | None,
        typer.Option(
            "--only",
            help="Keep only objects having these name prefixes",
        ),
    ] = None,
    exclude: Annotated[
        list[str] | None,
        typer.Option(
            "--exclude",
            help="Drop objects having these name prefixes",
        ),
    ] = None,
) -> None:
    data = dump.read_bytes()
    objects = parse_jitdump(data)
    if not objects:
        typer.echo("no JIT_CODE_LOAD records found", err=True)
        raise typer.Exit(code=1)

    seen: dict[str, int] = {}
    sections: list[tuple[str, int, bytes]] = []
    tiers: dict[str, int] = {}
    for o in _filter(objects, only or [], exclude or []):
        sn = section_name(o.name, seen)
        sections.append((sn, o.addr, o.code))
        t = tier_of(o.name)
        tiers[t] = tiers.get(t, 0) + 1

    if not sections:
        typer.echo("no objects after filtering", err=True)
        raise typer.Exit(code=1)

    write_elf(sections, str(output))

    total = sum(len(c) for _, _, c in sections)
    typer.echo(f"{len(sections)} code objects, {total} code bytes -> {output}", err=True)
    typer.echo("tier breakdown:", err=True)
    for tier, count in sorted(tiers.items(), key=lambda kv: -kv[1]):
        typer.echo(f"\t{tier}\t{count}", err=True)


if __name__ == "__main__":
    app()
