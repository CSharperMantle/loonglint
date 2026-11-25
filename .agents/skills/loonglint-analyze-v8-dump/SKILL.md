---
name: loonglint-analyze-v8-dump
description: >
  End-to-end analysis of V8 JIT output with LoongLint: capture a perf jitdump
  from a d8 run, convert it into a synthetic ELF, lint it, and read the tier
  provenance its symbols carry. Use whenever a task mentions --perf-prof,
  --perf-prof-path, jit-<pid>.dump, v8_jitdump2elf, or asks which peephole
  classes appear in Sparkplug (Baseline), Maglev, TurboFan, Ignition bytecode
  handlers, builtins, or wasm Liftoff/TurboFan generated code. Covers the
  analysis stage from dump capture through linting, plus the record-name grammar
  tiers are read from.
license: GPL-3.0-or-later
---

# LoongLint analysis of V8 JIT dumps

Intent: from one real run of a d8 workload, produce a complete jitdump, synthetic ELF(s) of its code objects, and LoongLint findings, with the tier provenance a report would need carried in the ELF's symbols. The pipeline is mechanical; the judgement is in reading that provenance.

## 1. Prime principle

Names, flags, and layouts here are shapes, not remembered facts. Confirm them against what you actually have: the d8 binary, the produced dump, each script's `--help`, and the linter's own output. If a name does not exist locally, the build or script version changed; adapt to the local artifacts instead of forcing the shape.

## 2. Canonical forms

Resolve every `<...>` locally. Options not shown are in `--help`.

```sh
# 1. Capture: run the workload with JitDump emission enabled
<d8> --perf-prof --perf-prof-path=<dumpdir> --logfile=<dumpdir>/v8.log <harness> -- <harness-flags>

# 2. Convert: one or more synthetic ELFs from the dump
uv run --project <loonglint>/script python <loonglint>/script/v8_jitdump2elf.py <dump> -o <elf>

# 3. Lint: findings on stdout, decode warnings on stderr
<loonglint-binary> <elf>
```

### Explanation of placeholders

- `<loonglint>` is the checkout that contains `script/`.
- `<loonglint-binary>` is the built linter.
- `<d8>` is the shell under test.
- `<harness>` is the workload, and `<harness-flags>` are its own options, which for d8 must follow `--`.
- `<dumpdir>`, `<dump>`, `<elf>` are this run's artifacts.

## 3. Step notes

**Capture.** Purpose: make the engine write a jitdump covering all compiled code objects. Check: a `jit-<pid>.dump` appears in the dump directory and holds a plausible number of records for the workload. Variation: the emission flag, its path flag, and any per-run granularity control belong to the local build; an already captured dump is equally valid input. The dump directory must exist before the run, and the run must be bounded and reach completion: the writer is buffered, so the final size only appears on a clean exit.

**Convert.** Purpose: expose each code object as an executable ELF section at its original JIT address, with the record name carried as a function symbol. Check: the converter's report of code objects, code bytes, and tier breakdown; the ELF's executable section count and symbols correspond to that report. Variation: the machine comes from the dump header, so there is no architecture selector to get wrong; prefix filters can isolate tiers, and a section-count limit splits large dumps into numbered chunks (all chunks must be linted).

**Lint.** Purpose: emit finding blocks and a summary for the machine code. Check: the trailing `<N> finding(s)` equals the sum of the per-rule counts. Variation: format selection, rule inclusion/exclusion, and label style are flags; consult `--help`. Exit status `1` means findings were found, not that the run failed; `2` is a usage or input error.

## 4. Operating lessons

- The dump switch implies other switches, and an implied switch cannot be turned off. Point the files they produce at the run directory; contradicting an implied switch is fatal. None of the implications changes instruction selection.
- The dump filename is per process. Use one directory per run, or track the newest file, when runs overlap.
- The tier mix is timing-dependent. The same script can produce different tiers across runs, and a short run may never reach the top tier; report the tiers the dump actually contains, and compare runs before drawing conclusions.
- Labels are not unique. Context-specialized copies and re-instantiated wasm modules repeat a name; code-range reuse also leaves two records claiming one start address, so labels there are ambiguous and the same bytes can be linted twice. Count unique sites by instruction address, and state the aliasing exposure you measured.
- The tier vocabulary is in the record name: JS markers for the JIT tiers, tags for builtins and bytecode handlers, untagged names for regexp and stub code, and a tier suffix for wasm.
- Patch slots and canonical spellings are engine- and architecture-specific. A word that looks removable may be a placeholder a patcher rewrites later, or a size-neutral idiom choice. Confirm against the emitter before calling it removable.
- Large dumps produce multi-gigabyte finding streams. Stream the analysis, and split the lint work by chunk rather than by loading everything.
- Decode warnings and the "scan incomplete" tail are diagnostics. A large skipped-word share suggests non-code bytes inside an executable section, since the machine is already fixed by the dump header.

## 5. Read next

- `references/pipeline.md` - capture, conversion, linting, tier provenance, scaling, and validation in detail.
- `references/findings-format.md` - the exact output grammar, both diff layouts, the V8 label hazards, and the summary trailer.
- `references/per-tier-attribution.md` - how to write a script that turns findings into a rule-by-tier table, and what such a script must decide per engine and architecture.

## 6. Symptoms and likely causes

- No dump: the emission flag is missing, the dump directory does not exist (the shell reports the failed path), or the build cannot emit a dump.
- Fatal flag error before any output: flags are validated up front, and a contradiction is fatal rather than resolved silently. The message names the flag and where the conflicting value came from.
- Empty or truncated ELF: wrong input, a run that was killed before the buffered writer flushed, or a chunking mistake.
- Everything in one tier bucket: the tier grammar drifted, or the bucket rule is too coarse.
- Totals disagree: summary rows parsed as findings, a diff layout missed, or filtered classes not subtracted.
- Different tier mixes across identical commands: tier-up races, not a dump defect.

## 7. Scope

This procedure covers the analysis stage: capture or accept a dump, convert, and lint. It ends at the finding stream; `references/per-tier-attribution.md` carries the next stage, the script that turns that stream into a per-tier table. Whatever consumes the table is outside this procedure.
