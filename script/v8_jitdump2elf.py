#!/usr/bin/env python3

# SPDX-License-Identifier: GPL-3.0-or-later
# pyright: reportUnusedCallResult=false

import struct
from collections.abc import Iterable, Iterator, Sequence
from pathlib import Path
from typing import Annotated, NamedTuple, final

import typer
from elftools.elf.constants import SHN_INDICES
from elftools.elf.enums import ENUM_E_MACHINE

from utils.elf import LOONGARCH64, ArchConfig, write_elf
from utils.riscv import riscv_config

# https://chromium.googlesource.com/v8/v8/+/f42af55a6e5b3b2dc4a98b589d36752b9da5b5bc/src/diagnostics/perf-jit.cc#77
JITDUMP_MAGIC = 0x4A695444

# https://chromium.googlesource.com/v8/v8/+/f42af55a6e5b3b2dc4a98b589d36752b9da5b5bc/src/diagnostics/perf-jit.cc#83
JIT_CODE_LOAD = 0


@final
class JitDumpHeader(NamedTuple):
    """C++ struct `PerfJitHeader`.

    https://chromium.googlesource.com/v8/v8/+/f42af55a6e5b3b2dc4a98b589d36752b9da5b5bc/src/diagnostics/perf-jit.cc#67
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
    """C++ struct `PerfJitBase`.

    https://chromium.googlesource.com/v8/v8/+/f42af55a6e5b3b2dc4a98b589d36752b9da5b5bc/src/diagnostics/perf-jit.cc#81
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
        # https://chromium.googlesource.com/v8/v8/+/f42af55a6e5b3b2dc4a98b589d36752b9da5b5bc/src/diagnostics/perf-jit.cc#95
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


def parse_jitdump(data: bytes) -> tuple[JitDumpHeader, list[CodeObject]]:
    """Return the jitdump header and all JIT_CODE_LOAD code objects."""
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
    return header, objects


# --- tier attribution -----------------------------------------------------

# https://chromium.googlesource.com/v8/v8/+/f42af55a6e5b3b2dc4a98b589d36752b9da5b5bc/src/objects/code-kind.cc#21
_JS_TIERS: Sequence[tuple[str, str]] = (
    ("o+", "Maglev (OSR)"),
    ("o*", "TurboFan (OSR)"),
    ("+'", "Maglev"),
    ("*'", "TurboFan"),
    ("+", "Maglev"),
    ("*", "TurboFan"),
    ("^", "Baseline"),
    ("~", "Interpreted"),
)


def tier_of(name: str) -> str:
    tag, sep, rest = name.partition(":")
    if sep and tag == "JS":
        for marker, tier in _JS_TIERS:
            if rest.startswith(marker):
                return tier
        return "JS"
    if tag in ("Builtin", "BytecodeHandler"):
        return tag
    return "Other"


# === ELF64 emission ===


def resolve_arch(elf_mach: int) -> ArchConfig:
    if elf_mach == ENUM_E_MACHINE["EM_LOONGARCH"]:
        return LOONGARCH64
    if elf_mach == ENUM_E_MACHINE["EM_RISCV"]:
        return riscv_config("rv64gc")
    typer.echo(
        f"Jitdump elf_mach {elf_mach} has no LoongLint backend (EM_LOONGARCH, EM_RISCV)", err=True
    )
    raise typer.Exit(code=1)


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
    help="Convert V8 --perf-prof JitDump files to an ELF for LoongLint consumption",
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
            default_factory=lambda: Path("v8jit.elf"),
            show_default="v8jit.elf",
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
    max_sections: Annotated[
        int,
        typer.Option(
            "--max-sections",
            min=1,
            max=SHN_INDICES.SHN_LORESERVE - 16,  # Reserve enough space
            help="Split output into chunks of at most this many sections",
        ),
    ] = SHN_INDICES.SHN_LORESERVE - 16,
) -> None:
    data = dump.read_bytes()
    header, objects = parse_jitdump(data)
    arch = resolve_arch(header.elf_mach)

    if not objects:
        typer.echo("no JIT_CODE_LOAD records found", err=True)
        raise typer.Exit(code=1)

    sections: list[tuple[str, int, bytes]] = []
    tiers: dict[str, int] = {}
    for o in _filter(objects, only or [], exclude or []):
        sections.append((o.name, o.addr, o.code))
        t = tier_of(o.name)
        tiers[t] = tiers.get(t, 0) + 1

    if not sections:
        typer.echo("no objects after filtering", err=True)
        raise typer.Exit(code=1)

    total = sum(len(c) for _, _, c in sections)
    typer.echo(f"{len(sections)} code objects, {total} code bytes", err=True)
    typer.echo("tier breakdown:", err=True)
    for tier, count in sorted(tiers.items(), key=lambda kv: -kv[1]):
        typer.echo(f"\t{tier}\t{count}", err=True)

    stem = output.stem
    suffix = output.suffix
    chunks = [sections[i : i + max_sections] for i in range(0, len(sections), max_sections)]
    for i, chunk in enumerate(chunks):
        if len(chunks) == 1:
            path = output
        else:
            path = output.with_name(f"{stem}.{i:03d}{suffix}")
        write_elf(chunk, str(path), arch)
        typer.echo(f"chunk {i}: {len(chunk)} objects -> {path}", err=True)


if __name__ == "__main__":
    app()
