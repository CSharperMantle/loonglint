# Findings format

The linter prints one block per finding, then a summary. The format is stable enough to parse, but two diff layouts exist and both appear in the same run.

## 1. Block header

```text
<file>:<label>+0x<offset>: <description> [<rule-id>]
```

or, when the finding address is not inside a function symbol:

```text
<file>:<label>:<absolute-address>: <description> [<rule-id>]
```

- `<file>` is the input path as given to the linter.
- `<label>` is a function symbol name for ELF input, or the region label for raw input.
- `+0x<offset>` is relative to the symbol start and is not zero padded. The absolute form is also not padded.
- `<description>` is human text and may contain punctuation.
- `<rule-id>` is `<arch>:<category>/<name>` and is the only reliably delimited field: everything between the last ` [` and the final `]`.

Do not parse the header by splitting on the first colon. The label can contain them. Anchor on the trailing bracket instead, and treat everything before the final `: ` preceding it as the locator.

V8 record names make the label hazardous:

- The tier marker adds `+`, `*`, `^`, `~`, and `'` to the name, and an OSR marker is `o+`, so the label contains plus signs that are not offsets.
- JS labels carry the script position after a space: `JS:*'<debug name> <script>:<line>:<column>`. Script names can contain spaces, parentheses, colons, brackets, and quotes; regexp labels embed the whole source and flags; a debug name can be empty, leaving a double space.
- Wasm labels are `JS:<name>-<index>-<liftoff|turbofan|none>`, where `<name>` is itself arbitrary text, and the index keeps the label unambiguous only within one module instance.

Consequences: anchor the relative offset on the `+0x<hex>:` immediately before the description, not on a bare `+` inside the label; and treat the label as a display string, not a key. Equal labels occur at different addresses (context-specialized copies, re-instantiated wasm), and equal addresses occur under different labels (reused code space).

## 2. Instruction lines

Every instruction line starts with a tab, then a two-character marker:

- `- ` removed
- `+ ` added
- `  ` kept (unchanged)

The rest of the line is tab-separated. There are two layouts.

Without a kept region, two address columns collapse to one per line:

```text
	- 0x0000000012345678	addi.d	$sp, $sp, 8
	+ 0x0000000012345678	addi.d	$sp, $sp, 16
```

With a kept region, old and new addresses are shown side by side, so each line has an extra column; the unused side is blank padding:

```text
	  0x0000000012345678	0x0000000012345678	ld.w	$a0, $sp, 12
	- 0x000000001234567c	                  	slli.w	$a0, $a0, 0
	+                   	0x000000001234567c	ld.wu	$a0, $sp, 12
```

Addresses are hexadecimal with `0x`, zero-padded to a fixed width that depends on the largest address in the finding (8 or 16 hex digits). The mnemonic and operands are separate tab fields; instructions without operands have only the mnemonic.

In a V8 dump a `(label, offset)` pair does not identify a site, because labels repeat. The instruction address does, except where two records cover the same bytes.

## 3. Summary trailer

```text
<N> finding(s)
	<count>	<rule-id>
	<count>	<rule-id>
Scan incomplete: <words> undecodable word(s) skipped; <bytes> trailing byte(s) ignored.
```

Counts are right-justified to the widest count and separated by tabs. The scan-completeness line is present only when applicable. There is no bracket and no address on these lines; use that to distinguish them from findings.

## 4. Robust parsing

1. Stream line by line; do not read a multi-gigabyte file into memory.
2. Strip ANSI escapes if colors may have been forced. Redirected output normally has none.
3. A header is a line that does not start with a tab and whose trimmed end is `]` after a `[rule-id]` suffix. Extract the rule id from the final bracketed span.
4. Instruction lines start with a tab. Remove the leading tab, split on tabs, and inspect the first field for the marker.
5. Find the first field matching `^0x[0-9a-f]+$` after the marker. That is the address of the instruction the line reports. A second matching field before the mnemonic belongs to the other side of the diff; ignore it.
6. The first field that is neither blank nor an address is the mnemonic; join the remaining fields as operands. Classifying a removed word as structural needs exactly this pair.
7. A blank line ends the block. Keep the removed lines associated with the immediately preceding header; never carry them across a blank line or a new header.
8. Stop parsing at the summary. Summary rows look like tab plus spaces plus digits and must not be counted as findings.

## 5. Cross-checks

- The summary total equals the number of headers.
- The per-rule counts in the summary equal the headers grouped by rule id.
- Every block contains at least one instruction line. A header with no following instruction lines indicates a parser error or a truncated stream.
- Distinct finding addresses are at most the number of blocks. Equal addresses are expected when two rules fire on the same instruction, and when two records alias the same code bytes.
