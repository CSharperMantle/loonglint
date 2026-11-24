# Pipeline detail

## 1. What the dump is

A JitDump is a container written by the engine during a run. It has a fixed header followed by length-tagged records; the records of interest to the linter are code loads, each carrying a name, a virtual address, a size, and the machine bytes. Everything the analysis needs about provenance (tier, function, wasm module, stub) is in that name. The container format is external; the names are engine-specific.

Do not hand-parse the container unless you must. The checkout's converter already walks it and reports what it found.

## 2. Capture

Required shape:

```sh
PERF_SPEW_DIR=<dumpdir> IONPERF=<mode> <js> <harness>
```

- `<dumpdir>` must exist and be writable; the engine creates the dump file inside it.
- The emission mode is enabled by the second variable. Without it, no dump is written even when the directory is set.
- The file is named per process, so concurrent runs in one directory are ambiguous. Use one directory per run, or select by modification time.
- The run must reach completion for the dump to be flushed. A killed process can leave a short or missing file.

What to verify: the file exists, its size grows during the run, and the final size is plausible relative to workload size and duration. For a small smoke workload a few hundred kilobytes is normal; large benchmarks produce tens to hundreds of megabytes.

If the documented names do not produce a dump, the build or its front end changed. Discover the local names empirically before concluding the build lacks support: the executable still embeds the literals it passes to the environment lookup, and the build configuration states whether the facility was compiled in. Use whatever search facility the machine offers; do not assume a particular tool. If a source checkout is available, its emitter and build files are authoritative; if not, the binary and a smoke run are enough.

An existing dump is valid input. There is no requirement to capture a new one.

## 3. Convert

```sh
uv run --project <loonglint>/script python <loonglint>/script/sm_jitdump2elf.py \
  <dump> -o <elf> --arch <arch>
```

The scripts live in the LoongLint checkout's Python project. Its declared dependencies are resolved by the project runner; do not install them globally or substitute a bare interpreter unless the environment already provides them.

Output shape, which the rest of the pipeline depends on:

- ELF executable, with one executable section per code record, placed at the record's original JIT address.
- One function symbol per record, spanning the record, carrying the record name. Findings are labelled through these symbols.
- Section names are not the reporting channel; symbols are.

Options worth knowing (confirm against `--help`):

- Architecture selector. It must match the build under test; a default may be another architecture. For targets whose rules are extension-gated, the selector may also accept an ISA string that is embedded into the ELF so the linter can gate rules correctly.
- Prefix inclusion and exclusion. Names begin with tier or subsystem tags, so prefixes isolate Ion, Baseline, Interpreter, Trampoline, wasm, or specific stubs.
- Section-count limit. A dump with more records than the limit is written as several files with a numeric infix; each is a normal ELF and each must be linted.

What to verify: the converter's record count, total code bytes, and tier breakdown are plausible for the dump; the ELF's executable sections and symbols correspond to that report.

Caveats:

- Records can overlap or repeat (recompilations, per-tier copies, shared stubs). Overlapping addresses are expected.
- Symbol lookup is by containing range; duplicate base addresses are collapsed, so one of several same-address records names the label.

## 4. Lint

```sh
<loonglint-binary> <elf>
```

- stdout is the finding stream followed by a summary: total count, per-rule counts, and a scan-completeness line when applicable.
- stderr carries per-region warnings about undecodable words and ignored trailing bytes. These are diagnostics, not findings.
- Exit status: `0` no findings, `1` findings present, `2` usage or input error. A wrapper that treats nonzero as failure will misreport every productive run.

Options worth knowing (confirm against `--help`):

- Input format selection; raw input additionally needs an architecture and optionally a base address.
- Rule inclusion and exclusion by identifier expression. Use this to isolate a class, or to skip a class that the analysis will subtract anyway.
- Symbol-label suppression, which falls back to section-address labels. Useful only when symbol names are unwanted.

What to verify: the reported total equals the sum of per-rule counts; the skipped-word share is small relative to decoded volume.

Limits: the scan is linear over executable sections, with no control-flow or data recognition. Bytes that decode as valid instructions can be reported. Treat implausible clusters inside data-like regions with suspicion before drawing conclusions.

## 5. Attribute

```sh
uv run --project <loonglint>/script python <loonglint>/script/sm_analyze_<arch>.py \
  <findings-file>
```

The stock analyzer reads the linter's stdout file and prints attribution tables. Read the analyzer that matches the architecture before using it: it defines how it derives tiers, what it subtracts from the actionable set, what options it takes, and what it prints. `--help` gives the interface; the script body gives the policy.

The attribute step needs a regular file path if the analyzer takes a path rather than a stream. Produce one by redirection, tee, a FIFO, or a local parser. The choice is free.

What to verify: actionable total equals the sum of actionable rows; every rule present in the lint summary appears in the attribution table, including rules the analyzer does not know.

## 6. Scaling

- Prefer per-tier filtering when you only care about one tier; it cuts conversion and lint time and makes labels unambiguous.
- Prefer section-count chunking when one ELF is too large to scan comfortably; lint chunks independently and concatenate findings.
- Per-chunk summaries are per chunk. Sum rule counts across chunks; do not read one chunk's summary as the run total.
- Findings streams can be several gigabytes. Analyze them as streams; compress at rest if they must be kept.
- Parallelism is per chunk or per filtered ELF. The linter itself is single-process per input.

## 7. End-to-end validation

1. Dump: exists, non-empty, plausible final size.
2. Conversion: record/byte/tier report plausible; ELF sections and symbols match.
3. Lint: summary total equals per-rule sum; skipped-word share small.
4. Attribution: actionable rows sum to the printed total; tiers match observed labels.
5. Cross-check: a handful of findings resolve to plausible symbols and offsets; duplicate addresses are understood, not accidental.

If any check fails, fix that stage before interpreting the tables. A wrong architecture selector, for example, converts successfully but produces both wrong decoding and wrong rule gating.
