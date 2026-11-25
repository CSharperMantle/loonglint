# Writing a per-tier attribution script

The checkout has no attribution script for V8. The SpiderMonkey analyzers that ship (`script/sm_analyze_*.py`) are keyed to that engine's vocabulary and policy, so they are models for the shape of such a script, but cannot be directly reused. This file describes how to write such a script: it turns a finding stream into a rule-by-tier table, in terms general enough to serve whichever engine produced the findings, and states what has to be decided before the table is trustworthy.

## 1. Read the converter first

The buckets are already encoded in the ELF: the converter derives them with `tier_of` from each record name, and the linter labels a finding with that name. Read `script/v8_jitdump2elf.py` end to end and answer:

1. What input does it consume, and in what form?
2. How does it derive a bucket from a record name?
3. Which marker spellings does it recognise, and in what order does it test them?
4. Which records fall into its catch-all buckets, and what lands there in practice?
5. What options does it accept, and what does each change?
6. What does it print, and in what shape?

The attribution script must agree with those answers. They belong in your notes, because a table is not comparable across runs unless the classification it used is stated. Do not carry answers from another checkout, another engine, or an earlier version.

## 2. Decide the policy

A script of this kind carries three decision tables, and each needs evidence:

- Bucket table: name grammar to bucket. The converter's rows are the starting point, but the grammar is longer than its buckets: tier markers are a prefix after the tag, wasm tiers are a suffix, and some records are untagged.
- Subtraction table: which reported classes are structural rather than removable. This is engine- and architecture-specific, and the only sound basis is the emitter that produced the word. A word a patcher rewrites later is structural; a size-neutral idiom change is not an allocation win.
- Site identity: labels repeat, and addresses can be claimed by more than one record. Decide whether the table counts findings or unique addresses, and print which.

Keep rules you do not recognize visible. Silent drops break the totals and hide new rules.

## 3. Write the script

- Stream the finding input; it can be very large.
- Parse with the documented grammar: anchor the relative offset on the `+0x<hex>:` immediately before the description.
- Classify by the name grammar, testing the tag, then the marker prefix longest first, then the wasm tier suffix. Substring tests over the whole label misclassify script names.
- Aggregate per rule and bucket, then print the table with a per-row total and a grand total.
- Print the policy that was applied, next to the table.
- Keep the bucket table and the subtraction table as separate per-engine and per-architecture data, so one script serves every dump the checkout's converters can produce.

The `script/sm_analyze_*.py` scripts are the in-tree examples of this shape: tier from the label prefix, one explicit subtraction table, reconciliation against the lint summary. Their tier prefixes and subtraction tables are that engine's, and do not transfer.

## 4. Validate

- Sum the per-rule counts across chunks and compare with the lint summaries; every rule's count must match before any subtraction is applied.
- Every rule present in a lint summary must appear in the table, including rules the bucket table does not know.
- Spot-check findings against the ELF: the address must fall inside a record, the label must match that record's name or one of the names at that address, and a subtracted word must be structural in the surrounding sequence.
