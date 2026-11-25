# Pipeline detail

## 1. What the dump is

A V8 jitdump is a perf JITDUMP container: a fixed header, then length-tagged records. The records that matter are code loads, each carrying a name, a virtual address, and the machine bytes. Provenance (tier, function, script position, wasm function and tier) is in the name, and the target machine is in the header, which is why the converter needs no architecture selector. The enabling flag disables code-space compaction, so a record's address stays valid during the run; freed space can still be reused later, which is why two records can claim one address (see Convert).

Names of JS code have this shape:

```text
JS:<marker><debug name> <script>:<line>:<column>
```

The marker is the tier, mapped in Tiers and provenance; the debug name can be empty; other kinds of code are tagged or carry a suffix, so their names are longer than this shape; very long names can be truncated.

Do not hand-parse the container unless you must. The checkout's converter already walks it and reports what it found.

## 2. Capture

Required shape:

```sh
<d8> --perf-prof --perf-prof-path=<dumpdir> --logfile=<dumpdir>/v8.log \
  <harness> -- <harness-flags>
```

- The path flag defaults to the current directory, so without it the dump lands next to the workload. The directory must already exist.
- The writer names the file `jit-<pid>.dump`, so concurrent runs in one directory are ambiguous. Use one directory per run, or select by modification time.
- The emission flag implies logging: a log file is written too, to the log flag's path. The implication cannot be turned off, which is why the shape above sets that path.
- The emission flag also implies disabling compact code space, which changes placement but not instruction selection.
- The shell warns about an unsupported configuration at startup. The warning describes the configuration the dump switch implies, not a fault.
- The run must reach completion for the dump to be flushed. The writer buffers on the order of a few megabytes, so the file grows in steps and its final size appears only on a clean exit.
- Harness options are the harness's, not the shell's. Under d8 they must follow a bare `--`, otherwise the shell's own parser warns about an unknown flag and drops them, and the harness silently runs its defaults.

What to verify: the file exists, its size is plausible relative to workload size and duration, and the converter reports a matching record count. For a small smoke workload a few megabytes is normal; a handful of JetStream tests produced a dump of a few hundred megabytes.

If the documented names do not produce a dump, the build or its front end changed. Discover the local names empirically before concluding the build lacks support: the executable still embeds the literals it passes to the flag parser, and the build configuration states whether the facility was compiled in. Use whatever search facility the machine offers; do not assume a particular tool. If a source checkout is available, its writer and flag definitions are authoritative; if not, the binary and a smoke run are enough.

An existing dump is valid input. There is no requirement to capture a new one.

## 3. Convert

```sh
uv run --project <loonglint>/script python <loonglint>/script/v8_jitdump2elf.py \
  <dump> -o <elf>
```

The scripts live in the LoongLint checkout's Python project. Its declared dependencies are resolved by the project runner; do not install them globally or substitute a bare interpreter unless the environment already provides them.

Output shape, which the rest of the pipeline depends on:

- ELF executable, with one executable section per code object, placed at the object's original JIT address.
- One function symbol per object, spanning the object, carrying the record name. Findings are labelled through these symbols.
- Section names are not the reporting channel; symbols are.
- The machine comes from the dump header; an unsupported machine is rejected.

Options worth knowing (confirm against `--help`):

- Prefix inclusion and exclusion. Names begin with the tag, so prefixes isolate every code object tagged `JS:`, or specific tiers by their marker, or drop builtins and bytecode handlers.
- Section-count limit. A dump with more objects than the limit is written as several files with a numeric infix; each is a normal ELF and each must be linted.

What to verify: the converter's object count, total code bytes, and tier breakdown are plausible for the dump; the ELF's executable sections and symbols correspond to that report.

Caveats:

- Record names are not unique: context-specialized copies and wasm modules re-instantiated per test produce equal names at different addresses.
- A start address can be claimed by more than one record, because freed code space is reused. Both records carry their own bytes and both are linted; the symbol covering a given byte is then ambiguous, and the same address can yield two finding sets. Counting repeated symbol values sizes the ambiguity.
- Symbol lookup is by containing range; nested records resolve to one of them, so a label can name a record other than the one whose bytes produced the finding.

## 4. Lint

```sh
<loonglint-binary> <elf>
```

- stdout is the finding stream followed by a summary: total count, per-rule counts, and a scan-completeness line when applicable.
- stderr carries per-region warnings about undecodable words and ignored trailing bytes. These are diagnostics, not findings.
- Exit status: `0` no findings, `1` findings present, `2` usage or input error. A wrapper that treats nonzero as failure will misreport every productive run.

Options worth knowing (confirm against `--help`):

- Input format selection; raw input additionally needs an architecture and optionally a base address.
- Rule inclusion and exclusion by identifier expression. Use this to isolate a class, or to skip a class already known to be structural.
- Symbol-label suppression, which falls back to section-address labels. Useful only when symbol names are unwanted.

What to verify: the reported total equals the sum of per-rule counts; the skipped-word share is small relative to decoded volume.

Limits: the scan is linear over executable sections, with no control-flow or data recognition. Bytes that decode as valid instructions can be reported. Records that alias an address are scanned once per record, so implausible doubles there are expected.

## 5. Tiers and provenance

A per-tier table needs provenance, and the ELF's symbols already carry it: the converter's `tier_of` names the buckets, and each record name carries the tier. The vocabulary:

| Name shape | Bucket |
| --- | --- |
| `JS:^...` | Baseline (Sparkplug) |
| `JS:+...`, `JS:+'...` | Maglev; `'` marks a context-specialized copy |
| `JS:*...`, `JS:*'...` | TurboFan; same marker suffix |
| `JS:o+...`, `JS:o*...` | Maglev and TurboFan OSR entries |
| `JS:~...` | interpreted trampoline copies, only when the run records interpreted frames on the native stack |
| `JS:<name>-<index>-liftoff/turbofan/none` | wasm code, tier in the suffix (the converter files it under `JS`) |
| `JS:` with no tier marker and no wasm suffix | adapters and wrappers (the converter's `JS` bucket) |
| `Builtin:...` | snapshot builtins, not workload JIT |
| `BytecodeHandler:...` | Ignition dispatch stubs |
| anything untagged (`RegExp.`, `Stub:`) | other |

Read the converter before relying on its rows: it collapses the context-specialized markers into their tier, and it puts every untiered `JS:` record, wasm bodies included, into one bucket, so that bucket is dominated by wasm. Separating wasm needs a split on the name suffix; wasm records land in the untiered `JS:` bucket, not in `other`.

The subtraction policy is engine- and architecture-specific, and the only sound basis is the emitter. Read the V8 source that emits the word, for the architecture under test: its assembler and macro assembler, and the compiler that called them. A word a fixed-shape sequence needs is structural; a size-neutral idiom change is not an allocation win.

`references/per-tier-attribution.md` describes the script that consumes findings this way.

What to verify: every rule in the lint summary appears in the table, including rules the vocabulary does not know; any subtraction adopted is backed by an emitter, not by analogy to another engine.

## 6. Scaling

- Prefer per-tier filtering when you only care about one tier; it cuts conversion and lint time and narrows the label set.
- Prefer section-count chunking when one ELF is too large to scan comfortably; lint chunks independently and concatenate findings.
- Per-chunk summaries are per chunk. Sum rule counts across chunks; do not read one chunk's summary as the run total.
- Findings streams can be several gigabytes. Analyze them as streams; compress at rest if they must be kept.
- Parallelism is per chunk or per filtered ELF. The linter itself is single-process per input.

## 7. End-to-end validation

1. Dump: exists, non-empty, plausible final size after a clean exit.
2. Conversion: object/byte/tier report plausible; ELF sections and symbols match.
3. Lint: summary total equals per-rule sum; skipped-word share small.
4. Provenance: the labels' vocabulary matches the buckets the converter reports; repeated names and repeated addresses are understood, not accidental.
