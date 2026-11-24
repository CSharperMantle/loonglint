---
name: loonglint-analyze-spidermonkey-dump
description: >
  End-to-end analysis of SpiderMonkey JIT output with LoongLint: capture a
  JitDump from a js shell run, convert it into a synthetic ELF, lint it, and
  attribute findings to JIT tiers and records. Use whenever a task mentions
  PERF_SPEW_DIR, IONPERF, jit-*.dump, sm_jitdump2elf, sm_analyze_*, or asks
  which peephole classes appear in Ion, Baseline, Interpreter, or wasm generated
  code. Covers the analysis stage from dump capture through tier attribution.
license: GPL-3.0-or-later
---

# LoongLint analysis of SpiderMonkey JIT dumps

Intent: from one real run of a js shell workload, produce a complete JitDump, synthetic ELF(s) of its code records, LoongLint findings, and a rule-by-tier table that separates actionable findings from forced or patch-slot shapes. The pipeline is mechanical; the judgement is in the attribution.

## 1. Prime principle

Names, flags, and layouts here are shapes, not remembered facts. Confirm them against what you actually have: the js binary, the produced dump, each script's `--help`, and the linter's own output. If a name does not exist locally, the build or script version changed; adapt to the local artifacts instead of forcing the shape.

## 2. Canonical forms

Resolve every `<...>` locally. Options not shown are in `--help`.

```sh
# 1. Capture: run the workload with dump emission enabled
PERF_SPEW_DIR=<dumpdir> IONPERF=<mode> <js> <harness>

# 2. Convert: one or more synthetic ELFs from the dump
uv run --project <loonglint>/script python <loonglint>/script/sm_jitdump2elf.py \
  <dump> -o <elf> --arch <arch>

# 3. Lint: findings on stdout, decode warnings on stderr
<loonglint-binary> <elf>

# 4. Attribute: rule x tier and actionable totals
uv run --project <loonglint>/script python <loonglint>/script/sm_analyze_<arch>.py \
  <findings-file>
```

### Explanation of placeholders

- `<loonglint>` is the checkout that contains `script/`.
- `<loonglint-binary>` is the built linter.
- `<js>` and `<harness>` are the shell and its workload.
- `<dumpdir>`, `<dump>`, `<elf>`, `<findings-file>` are this run's artifacts.
- `<arch>` selects the analyzer and converter target for the build under test.

## 3. Step notes

**Capture.** Purpose: make the engine write a JitDump covering all compiled records. Check: a dump file appears in the directory and grows while the run proceeds, and its final size is plausible for the workload. Variation: the exact environment names and modes, and any per-run granularity control, belong to the local build; an already captured dump is equally valid input. The dump directory must exist before the run.

**Convert.** Purpose: expose each code record as an executable ELF section at its original JIT address, with the record name carried as a function symbol. Check: the converter's report of records, code bytes, and tier breakdown; the ELF's executable section count and symbols correspond to that report. Variation: the architecture selector must match the build, prefix filters can isolate tiers, and a section-count limit splits large dumps into numbered chunks (all chunks must be linted). Do not rely on a default architecture.

**Lint.** Purpose: emit finding blocks and a summary for the machine code. Check: the trailing `<N> finding(s)` equals the sum of the per-rule counts. Variation: format selection, rule inclusion/exclusion, and label style are flags; consult `--help`. Exit status `1` means findings were found, not that the run failed; `2` is a usage or input error.

**Attribute.** Purpose: fold raw blocks into a rule-by-tier table and separate classes that are not actionable. Check: the analyzer's totals reconcile with the lint summary, and the tier buckets match the labels actually observed. Variation: the checkout may provide one analyzer per architecture; its options and policies are defined by that script, so read it and its `--help` before relying on its columns.

## 4. Operating lessons

- The dump filename is per process. Use one directory per run, or track the newest file, when runs overlap.
- `IONPERF` is what enables emission; its non-default modes add annotations or change record granularity, not the ELF pipeline.
- The converter writes its report to stderr, the same stream that can carry decode warnings later. Findings are the linter's stdout.
- Findings are labelled `symbol+offset`. Since the converter turns record names into symbols, the label prefix is the evidence for tier naming; reconcile it with the analyzer's tier list before trusting the columns.
- The linter scans every executable section. Records can repeat or overlap across tiers, so the same address can yield the same finding more than once; deduplicate on `(symbol, offset, rule)` when counting unique sites.
- Large dumps produce multi-gigabyte finding streams. Stream the analysis, and split the lint work by chunk rather than by loading everything.
- Decode warnings and the "scan incomplete" tail are diagnostics. A large skipped-word share suggests an architecture mismatch or data inside executable sections, not silently missing code.

## 5. Read next

- `references/pipeline.md` - capture, conversion, linting, scaling, and validation in detail.
- `references/findings-format.md` - the exact output grammar, both diff layouts, and the summary trailer.
- `references/analyzer-extension.md` - when and how to extend or replace the stock analyzer script.

## 6. Symptoms and likely causes

- No dump: the build lacks dump support, the enabling variable is wrong, or the directory is missing.
- Empty or truncated ELF: wrong input, wrong architecture selector, or a chunking mistake.
- Everything in one tier bucket: tier grammar drift, or the analyzer lacks that bucket.
- Totals disagree: summary rows parsed as findings, a diff layout missed, or filtered classes not subtracted.
- Huge skipped-word share: architecture mismatch, or non-code bytes inside an executable section.

## 7. Scope

This procedure covers the analysis stage: capture or accept a dump, convert, lint, attribute, and validate. It ends at the attribution report. Whatever consumes that report is outside this procedure; the report is shaped to be a usable input to it.
