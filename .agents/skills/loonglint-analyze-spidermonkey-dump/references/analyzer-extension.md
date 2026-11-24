# Extending or replacing the analyzer

The stock analyzer is a script in the LoongLint checkout. Read it before deciding anything about it. This file deliberately states no tier names, no filter lists, no options, and no output details; the script is the only authority for those, and it can change.

## 1. Read the script first

Read the analyzer end to end. While reading, answer:

1. What input does it consume, and in what form?
2. How does it derive attribution from a finding?
3. What classes does it subtract from the actionable set, and on what evidence?
4. What options does it accept, and what does each change?
5. What does it print, and in what shape?
6. What does it do with findings it does not recognize?
7. What assumptions does it make about the linter's output format?
8. How is it launched, and what dependency environment does it expect?

Every one of those answers belongs in your notes for the run, because the report is not comparable across runs unless the policy is stated. Do not carry answers from another checkout, another architecture, or an earlier version.

## 2. Choose the smallest intervention

- If a table or policy entry is missing, extend the script in place.
- If you need a different data shape (per-site rows, extra fields, a different grouping), post-process its output or write a local parser that shares the same input grammar.
- If the script's assumptions about the input no longer hold, fix the assumption at the source: either the parser or the analyzer, not both.

Do not fork the script for a one-off question; a local parser that emits a comparable summary is easier to compare and discard.

## 3. If you write a local parser

- Follow the documented finding grammar; treat the linter's summary as the reconciliation oracle.
- Stream the input. Finding streams can be very large.
- Keep rules you do not recognize visible. Silent drops break the totals and hide new rules.
- Record the attribution and subtraction policy you applied, in the report itself.
- Accept the raw dump as optional context only if you actually need provenance or neighbor bytes.

## 4. Validate the replacement

Run the stock analyzer and your replacement on the same findings file. Diff the totals. The only acceptable differences are the ones you intended. If the totals differ, the bug is in the replacement or in the stock script's assumption about the format; read both until it is clear which.
