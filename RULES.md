# Implemented rules in LoongLint

This documents peephole rules currently shipped in LoongLint, in `RuleManager` registration order.

## LoongArch rules

### `NopRule` (`loongarch:integer/nop`)

```asm
or Rd, Rd, $zero
# or
or Rd, $zero, Rd
# or
or Rd, Rd, Rd
# or
and Rd, Rd, Rd
# or
andn Rd, Rd, $zero
# or
xor Rd, Rd, $zero
# or
ori Rd, Rd, 0
# or
xori Rd, Rd, 0
# ->
# delete
```

#### Constraints

LA32/LA64. Width-independent logical forms.

* Every form writes the same value back to the same register with no side effects, and
* Width-independent because the base logical ops and logical-immediates do not sign-extend.
* The canonical NOP (`ANDI $zero, $zero, 0`) is intentionally not reported. NOPs are intentional padding with architectural special semantics.

`x | 0 = x`, `x | x = x`, `x & x = x`, `x & ~0 = x`, `x ^ 0 = x` at the native register width.

#### Evidence

* <https://github.com/gcc-mirror/gcc/blob/6afcc4f6da931eb93f3ab001a0dd9650ea71d1ea/gcc/config/loongarch/loongarch.md#L2659-L2659>
* LoongArch Reference Manual Volume 1, §2.2.1.10.

### `NopLA32Rule` (`loongarch:integer/nop-la32`)

```asm
add.w Rd, Rd, $zero
# or
sub.w Rd, Rd, $zero
# or
addi.w Rd, Rd, 0
# or
slli.w Rd, Rd, 0
# or
srli.w Rd, Rd, 0
# or
srai.w Rd, Rd, 0
# or
rotri.w Rd, Rd, 0
# ->
# delete
```

#### Constraints

LA32 only.

* Do not enable on LA64: `.W` operations sign-extend their 32-bit result on LA64, so these are identities only at the native 32-bit width.

`x + 0 = x`, `x - 0 = x`, `x << 0 = x`, `x >> 0 = x`, `x rotate 0 = x` at 32-bit width.

#### Evidence

* <https://github.com/gcc-mirror/gcc/blob/6afcc4f6da931eb93f3ab001a0dd9650ea71d1ea/gcc/config/loongarch/loongarch.md#L732-L732>

### `NopLA64Rule` (`loongarch:integer/nop-la64`)

```asm
add.d Rd, Rd, $zero
# or
sub.d Rd, Rd, $zero
# or
addi.d Rd, Rd, 0
# or
slli.d Rd, Rd, 0
# or
srli.d Rd, Rd, 0
# or
srai.d Rd, Rd, 0
# or
rotri.d Rd, Rd, 0
# ->
# delete
```

#### Constraints

LA64 only.

* `.D` operations do not sign-extend, so all listed forms are exact identities at 64-bit width.

`x + 0 = x`, `x - 0 = x`, `x << 0 = x`, `x >> 0 = x`, `x rotate 0 = x` at 64-bit width.

#### Evidence

* <https://github.com/gcc-mirror/gcc/blob/6afcc4f6da931eb93f3ab001a0dd9650ea71d1ea/gcc/config/loongarch/loongarch.md#L790-L790>

### `BitExtractRule` (`loongarch:integer/bit-extract`)

```asm
srli.d Rd, Rj, Lsb
andi Rd, Rd, Mask
# ->
bstrpick.d Rd, Rj, Msb, Lsb

# srai.d is accepted in place of srli.d; the low mask removes the
# replicated sign bits. The word form substitutes .w for .d.

# mask-first order:

andi Rd, Rj, Mask
srli.[wd] Rd, Rd, Lsb
# ->
bstrpick.[wd] Rd, Rj, Msb, Lsb

# here the mask covers the whole field: Msb = Len - 1 where
# Mask = (1 << Len) - 1, and Len must exceed Lsb. SRAI is not accepted
# in this order because nothing clears the replicated sign bits.
```

#### Constraints

`BSTRPICK.D` on LA64; `BSTRPICK.W` on LA32/LA64.

* `Mask = (1 << Len) - 1` must be a nonzero contiguous low-bit mask fitting `ANDI`'s 12-bit unsigned immediate, and
* In shift-first order `Msb = Lsb + Len - 1` and `Lsb + Len` must not exceed the selected width, and
* In mask-first order `Msb = Len - 1` and `Len` must exceed `Lsb`, and
* `Lsb` must be at least 1 so the pair never overlaps a NOP.

Shift-first extracts `Rj[Lsb+Len-1:Lsb]`; mask-first extracts `Rj[Len-1:Lsb]` because the AND has already cleared every bit above the field. Both forms extract the same low field that `BSTRPICK` extracts directly, with width-specific sign/zero extension.

#### Evidence

* <https://github.com/llvm/llvm-project/blob/37b7c17388717199e9669e3ea5bb2a5c9711bbb1/llvm/lib/Target/LoongArch/LoongArchISelLowering.cpp#L6362-L6413>

### `ZeroExtendRule` (`loongarch:integer/zero-extend`)

```asm
slli.[wd] Rd, Rj, Shamt
srli.[wd] Rd, Rd, Shamt
# ->
bstrpick.[wd] Rd, Rj, Msb, 0

# where Msb = width - Shamt - 1
```

#### Constraints

`BSTRPICK.D` on LA64; `BSTRPICK.W` on LA32/LA64.

* `Shamt` must be in `[1, width - 1]`, and
* The second shift must reuse the first destination with the same amount.

A logical left shift then logical right shift by the same amount clears the high `width - Shamt` bits, which `BSTRPICK` with `msb = width - Shamt - 1`, `lsb = 0` does directly as a zero-extension.

#### Evidence

* <https://github.com/llvm/llvm-project/blob/37b7c17388717199e9669e3ea5bb2a5c9711bbb1/llvm/lib/Target/LoongArch/LoongArchISelLowering.cpp#L6380-L6413>

### `BitReverseRule` (`loongarch:integer/bit-reverse`)

```asm
revb.2w Rd, Rj
bitrev.w Rd, Rd
# ->
bitrev.4b Rd, Rj

# either instruction order is accepted; the LA64 doubleword form is:
revb.d + bitrev.d  ->  bitrev.8b   (either order)
```

#### Constraints

LA64 only.

* The second instruction must overwrite the first's destination, and
* Keep `.w`/`.d` and `.4b`/`.8b` widths paired.

A byte-within-word reversal composed with a whole-word bit reversal leaves bit reversal within each byte.

#### Evidence

* <https://github.com/llvm/llvm-project/blob/37b7c17388717199e9669e3ea5bb2a5c9711bbb1/llvm/lib/Target/LoongArch/LoongArchInstrInfo.td#L1919-L1925>

### `ByteReverseRule` (`loongarch:integer/byte-reverse`)

```asm
revb.4h Rd, Rj
revh.d Rd, Rd
# ->
revb.d Rd, Rj

# either instruction order is accepted.
```

#### Constraints

LA64 only.

* The second instruction must overwrite the first's destination.

A byte-within-halfword reversal composed with a halfword-order reversal is a whole-doubleword byte reversal, which `REVB.D` performs directly.

#### Evidence

* <https://github.com/llvm/llvm-project/blob/37b7c17388717199e9669e3ea5bb2a5c9711bbb1/llvm/lib/Target/LoongArch/LoongArchInstrInfo.td#L1919-L1925>

### `MulhSextRule` (`loongarch:integer/mulh-sext`)

```asm
mulh.w[u] Rd, Rj, Rk
addi.w Rd, Rd, 0
# ->
mulh.w[u] Rd, Rj, Rk   # delete the extension

# or

mulh.w[u] Rd, Rj, Rk
slli.w Rd, Rd, 0
# ->
mulh.w[u] Rd, Rj, Rk   # delete the extension
```

#### Constraints

LA64 only; on LA32 the same-destination extension is a NOP handled by `NopLA32Rule`.

* Match only same-destination word sign-extension idioms (`ADDI.W Rd, Rd, 0` or `SLLI.W Rd, Rd, 0`) after either `MULH.W` or `MULH.WU`.

`MULH.W` and `MULH.WU` already sign-extend their 32-bit high result to the GPR width on LA64, so the following same-destination extension is redundant.

#### Evidence

* <https://github.com/gcc-mirror/gcc/blob/6afcc4f6da931eb93f3ab001a0dd9650ea71d1ea/gcc/config/loongarch/loongarch.md#L986-L1001>

### `ShiftChainRule` (`loongarch:integer/shift-chain`)

```asm
slli.d Rd, Rj, Shamt0
slli.d Rd, Rd, Shamt1
# ->
slli.d Rd, Rj, Shamt

# srli.[wd] compose the same way; srai.[wd] clamps the combined amount
# to the width maximum instead of rejecting overflow. Word forms
# substitute .w for .d and 31 for 63.
```

#### Constraints

`.D` on LA64; `.W` on LA32/LA64.

* the second destination must overwrite the first, and
* the combined amount must stay in the replacement's legal range (clamped for arithmetic right shifts), and
* both amounts must be nonzero so the pair never overlaps a NOP.

Fixed-width immediate shifts compose by adding amounts; arithmetic right shifts saturate at the width maximum.

#### Evidence

* <https://github.com/llvm/llvm-project/blob/37b7c17388717199e9669e3ea5bb2a5c9711bbb1/llvm/lib/Target/LoongArch/LoongArchInstrInfo.td#L1250-L1264>

### `ShiftDoubleRule` (`loongarch:integer/shift-self-add`)

```asm
add.d Rd, Rj, Rj
slli.d Rd, Rd, Shamt
# ->
slli.d Rd, Rj, ShamtPlusOne

# or

slli.d Rd, Rj, Shamt
add.d Rd, Rd, Rd
# ->
slli.d Rd, Rj, ShamtPlusOne

# word forms substitute .w for .d and 30 for 62 as the max Shamt.
```

#### Constraints

`.D` on LA64; `.W` on LA32/LA64.

* `ShamtPlusOne = Shamt + 1` must be encodable (`Shamt <= 62` for `.D`, `<= 30` for `.W`), and
* the same destination in both instructions keeps the temporary dead without a liveness proof.

Doubling a fixed-width shifted value equals shifting by one additional bit at that width.

#### Evidence

* <https://github.com/llvm/llvm-project/blob/37b7c17388717199e9669e3ea5bb2a5c9711bbb1/llvm/lib/Target/LoongArch/LoongArchInstrInfo.td#L652-L672>

### `RotateCombineRule` (`loongarch:integer/rotate-combine`)

```asm
rotri.d Rd, Rj, Shamt0
rotri.d Rd, Rd, Shamt1
# ->
rotri.d Rd, Rj, Shamt

# word forms substitute .w for .d and 32 for 64;
# Shamt = (Shamt0 + Shamt1) modulo width
```

#### Constraints

`.D` on LA64; `.W` on LA32/LA64.

* Both destinations must be the same register, and
* Both amounts must be nonzero so the pair never overlaps a NOP.

Immediate rotations compose modulo the operand width.

#### Evidence

* <https://github.com/llvm/llvm-project/blob/37b7c17388717199e9669e3ea5bb2a5c9711bbb1/llvm/lib/Target/LoongArch/LoongArchInstrInfo.td#L1290-L1292>

### `ShiftMaskRule` (`loongarch:integer/shift-mask`)

```asm
andi Rd, CountRj, Mask
sll.[wd] Rd, ValueRj, Rd
# ->
sll.[wd] Rd, ValueRj, CountRj

# the same shape applies to srl.[wd], sra.[wd], and rotr.[wd];
# any mask keeping every count bit is accepted:
# (Mask & (Width - 1)) == Width - 1

# or, on LA64, the count producer may be a BSTRPICK.D:

bstrpick.d Rd, CountRj, Msb, 0   # Msb >= 5 for .D shifts, >= 4 for .W shifts
sll.[wd] Rd, ValueRj, Rd
# ->
sll.[wd] Rd, ValueRj, CountRj
```

#### Constraints

`.W` on LA32/LA64; `.D` on LA64.

* The mask destination must be overwritten by the shift, and
* `ValueRj` must not alias `Rd`, or removing the mask would change the shifted value, and
* An `ANDI` count mask must keep every count bit, and
* A `BSTRPICK.D` count producer must have `lsb` 0 and keep at least the count bits (`Msb + 1 >= log2(Width)`).

LoongArch variable shifts already consume only the low 5 (`.W`) or 6 (`.D`) shift-count bits, so any count masking that preserves those bits is redundant.

#### Evidence

* <https://github.com/llvm/llvm-project/blob/37b7c17388717199e9669e3ea5bb2a5c9711bbb1/llvm/lib/Target/LoongArch/LoongArchISelDAGToDAG.cpp#L283-L340>

### `AndNotRule` (`loongarch:integer/and-not`)

```asm
nor Rd, NotRk, $zero
and Rd, AndRj, Rd
# ->
andn Rd, AndRj, NotRk

# all four operand-order combinations of the NOR $zero operand and
# the AND temp operand are accepted.
```

#### Constraints

LA32/LA64.

* The `AND` must overwrite the `NOR` temporary, and
* Neither `NotRk` nor `AndRj` may alias that temporary, and
* Both `NOR`/`AND` operand orders are handled.

`ANDN` is precisely `rj & ~rk`.

#### Evidence

* <https://github.com/llvm/llvm-project/blob/37b7c17388717199e9669e3ea5bb2a5c9711bbb1/llvm/lib/Target/LoongArch/LoongArchInstrInfo.td#L1421-L1421>

### `OrNotRule` (`loongarch:integer/or-not`)

```asm
nor Rd, NotRk, $zero
or Rd, OrRj, Rd
# ->
orn Rd, OrRj, NotRk

# all four operand-order combinations are accepted.
```

#### Constraints

LA32/LA64.

* The `OR` must overwrite the `NOR` temporary, and
* Neither `NotRk` nor `OrRj` may alias that temporary, and
* Both `NOR`/`OR` operand orders are handled.

`ORN` is precisely `rj | ~rk`.

#### Evidence

* <https://github.com/llvm/llvm-project/blob/37b7c17388717199e9669e3ea5bb2a5c9711bbb1/llvm/lib/Target/LoongArch/LoongArchInstrInfo.td#L1420-L1420>

### `NotOrRule` (`loongarch:integer/not-or`)

```asm
or Rd, Rj, Rk
nor Rd, Rd, $zero
# ->
nor Rd, Rj, Rk

# both OR operand orders and both NOR $zero operand orders are accepted.
```

#### Constraints

LA32/LA64.

* The `NOR` must overwrite the `OR` temporary, and
* `OR` operand order is interchangeable.

`NOR` is the one-instruction form of `NOT(OR(...))`.

#### Evidence

* <https://github.com/llvm/llvm-project/blob/37b7c17388717199e9669e3ea5bb2a5c9711bbb1/llvm/lib/Target/LoongArch/LoongArchInstrInfo.td#L1419-L1419>

### `BitCountRule` (`loongarch:integer/bit-count`)

```asm
nor NotRd, Rj, $zero
clz.w Rd, NotRd
# ->
clo.w Rd, Rj

# ctz.w -> cto.w; the .d forms substitute .d for .w.
# the NOR $zero operand may also be nor NotRd, $zero, Rj.
```

#### Constraints

`.W` on LA32/LA64; `.D` on LA64.

* The count instruction must overwrite the `NOR` temporary, and
* Do not infer a complement through an arbitrary intervening instruction.

Leading/trailing zero counts of `~x` equal leading/trailing one counts of `x` at the same width.

#### Evidence

* <https://github.com/llvm/llvm-project/blob/37b7c17388717199e9669e3ea5bb2a5c9711bbb1/llvm/lib/Target/LoongArch/LoongArchInstrInfo.td#L1451-L1458>

### `BranchToNextRule` (`loongarch:control/branch-to-next`)

```asm
b 4
# or any non-linking direct conditional branch to PC + 4
# ->
# delete
```

#### Constraints

LA32/LA64.

* The branch must be non-linking and direct, and
* Its resolved target must be the immediately following instruction, and
* Calls and indirect branches are excluded.

A branch to PC + 4 changes neither architectural state nor control flow.

#### Evidence

* <https://github.com/llvm/llvm-project/blob/37b7c17388717199e9669e3ea5bb2a5c9711bbb1/llvm/include/llvm/MC/MCInstrAnalysis.h#L187>

### `DegenerateBranchRule` (`loongarch:control/degenerate-branch`)

```asm
# always-true forms:
beq Rj, Rj, Offs
# or bge / bgeu on r, r, or beqz $zero
# ->
b Offs

# always-false forms:
bne Rj, Rj, Offs
# or blt / bltu on r, r, or bnez $zero
# ->
# delete
```

#### Constraints

LA32/LA64.

* Preserve direct target and branch range, and
* A target equal to PC + 4 is left to `BranchToNextRule` so the two rules never double-report.

Integer comparisons of a register with itself, or zero with zero, have constant truth values.

#### Evidence

* <https://github.com/llvm/llvm-project/blob/37b7c17388717199e9669e3ea5bb2a5c9711bbb1/llvm/include/llvm/MC/MCInstrAnalysis.h#L57-L82>

### `ShiftAddAlslDRule` (`loongarch:integer/shift-add-alsl-d`)

```asm
slli.d SlliRd, SlliRj, Shamt
add.d SlliRd, AddRj, SlliRd
# ->
alsl.d SlliRd, SlliRj, AlslRk, Shamt

# or

slli.d SlliRd, SlliRj, Shamt
add.d SlliRd, SlliRd, AddRk
# ->
alsl.d SlliRd, SlliRj, AlslRk, Shamt
```

#### Constraints

LA64 only.

* `Shamt` is 1--4, and
* `SlliRd` is not `$zero`, and
* `AlslRk` is the non-temporary `ADD.D` source and must not alias `SlliRd`.
* The producer temporary is consumed by the overwriting `ADD.D`.
* Both `ADD.D` operand orders are accepted.

`ALSL.D` reads both original sources before writing. It is not equivalent when the second `ADD.D` source aliases the shifted temporary.

#### Evidence

* <https://github.com/llvm/llvm-project/blob/37b7c17388717199e9669e3ea5bb2a5c9711bbb1/llvm/lib/Target/LoongArch/LoongArchInstrInfo.td#L1480-L1490>

### `AddiPairRule` (`loongarch:integer/addi-pair`)

```asm
addi.w Rd, Rj, Imm0
addi.w Rd, Rd, Imm1
# ->
addi.w Rd, Rj, Combined

# on LA64, substitute addi.d for addi.w.
```

#### Constraints

`.W` on LA32/LA64; `.D` on LA64.

* the second destination must overwrite the first, and
* both immediates must be nonzero (an `ADDI`-by-0 is a NOP only when `Rd == Rj`; zero-immediate members stay excluded), and
* the combined immediate must fit the replacement's signed 12-bit range.

Fixed-width addition is associative modulo the operation width; no memory or control state is involved.

#### Evidence

* <https://github.com/llvm/llvm-project/blob/37b7c17388717199e9669e3ea5bb2a5c9711bbb1/llvm/lib/Target/LoongArch/LoongArchInstrInfo.td#L1318-L1330>

### `AddressLoadRule` (`loongarch:memory/address-load`)

```asm
addi.d Rd, Rj, AddSi12
ld.{b,h,w,d,bu,hu,wu} Rd, Rd, LoadOffset
# or
ldptr.[wd] Rd, Rd, LoadOffset
# ->
ld.{b,h,w,d,bu,hu,wu} Rd, Rj, CombinedOffset
# or
ldptr.[wd] Rd, Rj, CombinedOffset
```

#### Constraints

`ADDI.D` address arithmetic with all listed loads on LA64; `ADDI.W` address arithmetic with `LD.{B,H,W,BU,HU}` on LA32.

* The load data width is independent of the address-calculation width.
* For ordinary loads, `CombinedOffset = AddSi12 + LoadOffset` must fit the signed 12-bit byte offset.
* For `LDPTR.{W/D}`, `CombinedOffset = AddSi12 + LoadOffset` must be a signed 14-bit value shifted left by 2.
* The address temporary must not be `$zero`.
* The load destination must equal the address temporary, proving that the temporary is overwritten.

For `LDPTR.{W/D}`, decoded `MCOperand` immediates are byte offsets even though the encoded `si14` field is shifted left by 2; validate the combined byte offset with the shifted-14-bit constraint and keep that byte offset in the replacement `MCInst`.

`ADDI.W` on LA64 is intentionally excluded: without an address/sign-extension proof, it is not interchangeable with the 64-bit address calculation.

One base-register addition plus one immediate-addressed load folds into the load displacement when the effective address and destination aliasing are preserved. Distinct-destination forms remain deferred because they require liveness analysis.

#### Evidence

* <https://github.com/llvm/llvm-project/blob/37b7c17388717199e9669e3ea5bb2a5c9711bbb1/llvm/lib/Target/LoongArch/LoongArchInstrInfo.cpp#L917-L998>

### `LoadExtendRule` (`loongarch:memory/load-extend`)

```asm
ld.b Rd, Rj, Si12
ext.w.b Rd, Rd
# ->
ld.b Rd, Rj, Si12   # delete the extension

# or ld.h + ext.w.h (LA32/LA64); ld.w + addi.w Rd, Rd, 0 or
# slli.w Rd, Rd, 0 (LA64 only); or ldptr.w + addi.w/slli.w
# Rd, Rd, 0 (LA64 only).
```

#### Constraints

`EXT.W.B`/`EXT.W.H` on LA32/LA64; the `LD.W` and `LDPTR.W` forms on LA64 only (on LA32 the `ADDI.W`/`SLLI.W`-by-0 identities are handled by `NopLA32Rule`).

* The extension destination must be the load destination.

The signed loads and `LDPTR.W` already perform the requested sign extension.

#### Evidence

* <https://github.com/llvm/llvm-project/blob/37b7c17388717199e9669e3ea5bb2a5c9711bbb1/llvm/lib/Target/LoongArch/LoongArchInstrInfo.td#L1949-L1953>

### `IndexedLoadRule` (`loongarch:memory/indexed-load`)

```asm
add.d Rd, Rj, Rk
ld.* Rd, Rd, 0
# ->
ldx.* Rd, Rj, Rk
```

#### Constraints

LA64 only.

* The address temporary must not be `$zero`, and
* The address sources `Rj`/`Rk` must not alias `Rd`, since the indexed load reads them after the `ADD.D` has written the temporary.

One base-plus-index addition followed by a zero-offset load folds into the indexed load when the index sources are preserved.

#### Evidence

* <https://github.com/llvm/llvm-project/blob/37b7c17388717199e9669e3ea5bb2a5c9711bbb1/llvm/lib/Target/LoongArch/LoongArchInstrInfo.td#L1969-L1978>

### `LoadZeroExtendRule` (`loongarch:memory/load-zero-extend`)

```asm
ld.[bhw] Rd, Rj, Si12
bstrpick.d Rd, Rd, {7,15,31}, 0
# ->
ld.{bu,hu,wu} Rd, Rj, Si12   # LA64

# on LA32, ld.[bh] + bstrpick.w Rd, Rd, {7,15}, 0 -> ld.{bu,hu}
```

#### Constraints

`BSTRPICK.D` on LA64; `BSTRPICK.W` on LA32 (`BSTRPICK.W` sign-extends on LA64 and would not zero-extend there).

* The `BSTRPICK` must extract the full loaded low field (`msb` of 7/15/31 with `lsb = 0`) and overwrite the load destination.

Extracting the loaded low field with `BSTRPICK` is exactly what the unsigned loads `LD.BU`/`LD.HU`/`LD.WU` produce.

#### Evidence

* <https://github.com/llvm/llvm-project/blob/37b7c17388717199e9669e3ea5bb2a5c9711bbb1/llvm/lib/Target/LoongArch/LoongArchInstrInfo.td#L1954-L1959>

### `UnsignedLoadPickRule` (`loongarch:memory/unsigned-load-pick`)

```asm
ld.bu Rd, Rj, Si12
bstrpick.d Rd, Rd, 7, 0  # LA64
# ->
ld.bu Rd, Rj, Si12

# On LA32 the pick is bstrpick.w Rd, Rd, {7,15}, 0 after ld.{bu,hu}.
```

#### Constraints

`BSTRPICK.D` picks on LA64; `BSTRPICK.W` picks on LA32. On LA32 `BSTRPICK.W` zero-extends the field across the whole 32-bit GRLEN.

* The `BSTRPICK` must extract exactly the loaded width (`msb` of 7/15/31 with `lsb = 0`) and overwrite the load destination.

An unsigned load already produces the zero-extended field, so a full-width `BSTRPICK` of the same field rewrites the same value and can be deleted.

## RISC-V rules

### `NopRule` (`riscv:integer/nop`)

```asm
addi Rd, Rd, 0
# or
andi Rd, Rd, -1
# or
ori Rd, Rd, 0
# or
xori Rd, Rd, 0
# or
slli Rd, Rd, 0
# or
srli Rd, Rd, 0
# or
srai Rd, Rd, 0
# or
add Rd, Rd, x0
# or
add Rd, x0, Rd
# or
sub Rd, Rd, x0
# or
sll Rd, Rd, x0
# or
srl Rd, Rd, x0
# or
sra Rd, Rd, x0
# or
xor Rd, Rd, x0
# or
xor Rd, x0, Rd
# or
or Rd, Rd, x0
# or
or Rd, x0, Rd
# or
or Rd, Rd, Rd
# or
and Rd, Rd, Rd
# ->
# delete

c.addi Rd, 0
# or
c.slli Rd, 0
# or
c.srli Rdprime, 0
# or
c.srai Rdprime, 0
# or
c.andi Rdprime, -1
# or
c.mv Rd, Rd
# or
c.and Rdprime, Rdprime
# or
c.or Rdprime, Rdprime
# ->
# delete
```

#### Constraints

RV32/RV64 base ISA plus Zca compressed forms; width-independent.

* Every form writes the register's own value back to itself (`x + 0 = x`, `x - 0 = x`, `x << 0 = x`, `x | 0 = x`, `x | x = x`, `x & x = x`, `x & ~0 = x`, `x ^ 0 = x` at the native register width; the compressed forms expand to `addi rd, rd, 0`, `slli/srli/srai rd, rd, 0`, `andi rd, rd, -1`, `add rd, x0, rd`, `and/or rd, rd, rd`).
* The register-prime forms (`rdprime`) operate on the compressed x8--x15 window, which never includes `x0`.
* Encodings with rd=x0 are never reported. The ISA manual reserves them as HINTs (RV32I/RV64I HINT tables and the Zca HINT table): the canonical 4-byte NOP (`addi x0, x0, 0`, encoding `0x00000013`), the canonical 2-byte c.nop (encoding `0x0001`), the semihosting markers (`slli x0, x0, 31`, `srai x0, x0, 7`), the NTL family (`add x0, x0, x2`--`x5`, `c.add x0, x2`--`x5`) and the pause fence live there, and their operand fields carry the hint payload. Reporting them could prompt deleting a deliberate hint.
* The compressed copy-to-self HINTs (`c.addi rd, 0`, `c.slli rd, 0`, `c.srli/c.srai rdprime, 0`) are reported: the manual describes them as "rd overwritten with a copy of itself", no standard toolchain emits them as hints, and implementations must ignore HINTs, so deletion is architecturally inert.
* W-suffix forms are never matched: on RV64 they sign-extend the word result, which is not a NOP (`addiw rd, rd, 0` is the canonical `sext.w`, and `c.addiw rd, 0` prints as `sext.w`); on RV32 they do not exist.
* `c.add rd, rd` is not a NOP: it expands to `add rd, rd, rd` (doubling), and `rs2=x0` encodes `c.jalr`/`c.ebreak` instead. `c.xor rdprime, rdprime` zeroes. `c.lui`, `c.li rd, 0` and `c.addi16sp` with zero immediates are reserved encodings or zeroing forms.

### `ZbaNopRule` (`riscv:integer/zba-nop`)

```asm
sh1add/sh2add/sh3add Rd, X0, Rd
# or
add.uw/sh1add.uw/sh2add.uw/sh3add.uw Rd, X0, Rd
# ->
# delete
```

#### Constraints

Zba; gated on `hasZba()`. rd != x0, the shifted/zero-extended operand is `X0`, and the addend is the destination: `(0 << N) + rd = rd` and `zext32(0) + rd = rd`.

#### Evidence

* <https://github.com/llvm/llvm-project/blob/37b7c17388717199e9669e3ea5bb2a5c9711bbb1/llvm/lib/Target/RISCV/RISCVInstrInfoZb.td#L754-L773>

### `ZbbNopRule` (`riscv:integer/zbb-nop`)

```asm
min/minu/max/maxu Rd, Rd, Rd
# or
rol/ror Rd, Rd, X0
# or
rori Rd, Rd, 0
# or
andn Rd, Rd, X0
# ->
# delete
```

#### Constraints

Zbb; the rotate/andn arms also apply under Zbkb (`shouldRun` is `hasZbb() || hasZbkb()`, and the min/max arms additionally check `hasZbb()`). rd != x0, per the NopRule policy. min/max of a value with itself is the value, rotate by zero is a NOP, and `rd & ~0 = rd`.

#### Evidence

* <https://github.com/llvm/llvm-project/blob/37b7c17388717199e9669e3ea5bb2a5c9711bbb1/llvm/lib/Target/RISCV/RISCVInstrInfoZb.td#L481-L499>
* <https://github.com/llvm/llvm-project/blob/37b7c17388717199e9669e3ea5bb2a5c9711bbb1/llvm/lib/Target/RISCV/RISCVInstrInfoZb.td#L608-L611>

### `ZbsNopRule` (`riscv:integer/zbs-nop`)

```asm
bclri Rd, Rs, N
bclri Rd, Rd, N
# ->
# delete the second

bseti Rd, Rs, N
bseti Rd, Rd, N
# ->
# delete the second

binvi Rd, Rs, N
binvi Rd, Rd, N
# ->
addi Rd, Rs, 0 (mv)
# or delete the pair when Rd == Rs
```

#### Constraints

Zbs; gated on `hasZbs()`. Same-destination chain (the first destination is the second destination and source), the same immediate N in both instructions, and rd != x0. `bclri`/`bseti` are idempotent; `binvi` is an involution.

#### Evidence

* <https://github.com/llvm/llvm-project/blob/37b7c17388717199e9669e3ea5bb2a5c9711bbb1/llvm/lib/Target/RISCV/RISCVInstrInfoZb.td#L327-L331>
* <https://github.com/gcc-mirror/gcc/blob/6afcc4f6da931eb93f3ab001a0dd9650ea71d1ea/gcc/config/riscv/bitmanip.md#L854-L854>
### `SextWRule` (`riscv:integer/sext-w`)

```asm
addw Rd, Rs1, Rs2
addiw Rd, Rd, 0
# ->
# delete the addiw

slli Rd, Rs, 32
srai Rd, Rd, 32
# ->
addiw Rd, Rs, 0
```

#### Constraints

RV64 base ISA. The explicit `sext.w` (`addiw Rd, Rd, 0`, also encoded as `c.addiw Rd, 0`) is redundant when the producer already sign-extends its word result: `addw`, `addiw`, `subw`, `sllw`, `slliw`, `srlw`, `srliw`, `sraw`, `sraiw`, `mulw`, `divw`, `divuw`, `remw`, `remuw`, `slt`/`slti`/`sltu`/`sltiu`, `lui`, `lb`/`lh`/`lw`/`lbu`/`lhu`, `fmv.x.w`, and the A-extension W-forms (LLVM's `IsSignExtendingOpW` TSFlag, which the rule reads directly; `c.addiw` is added explicitly because compressed instructions carry no flag). `lwu` is excluded: it zero-extends, so a following `sext.w` changes the value. `auipc` is excluded.

* The producer must write the `addiw` destination, and rd=x0 stays excluded by the NopRule HINT policy.
* The shift-pair arm needs the same destination and the `XLEN-32` amounts (`slli 32; srai 32` is the generalized 32-bit sign-extension idiom).

#### Evidence

* <https://github.com/llvm/llvm-project/blob/37b7c17388717199e9669e3ea5bb2a5c9711bbb1/llvm/lib/Target/RISCV/RISCVOptWInstrs.cpp#L11-L13>
* <https://github.com/llvm/llvm-project/blob/37b7c17388717199e9669e3ea5bb2a5c9711bbb1/llvm/lib/Target/RISCV/RISCVInstrInfo.td#L970-L994>
* <https://github.com/gcc-mirror/gcc/blob/6afcc4f6da931eb93f3ab001a0dd9650ea71d1ea/gcc/config/riscv/riscv.md#L1954-L1970>

### `ZbaZextWRule` (`riscv:integer/zba-zext-w`)

```asm
slli Rd, Rs, 32
srli Rd, Rd, 32
# ->
add.uw Rd, Rs, X0 (zext.w)

add.uw Rd, Rd, X0
# after a producer already known to fit in 32 bits
# ->
# delete

add.uw Tmp, Rs1, X0
add Rd, Tmp, Rs2
# or add Rd, Rs2, Tmp
# ->
add.uw Rd, Rs1, Rs2

add.uw Tmp, Rs1, X0
sh1add Rd, Tmp, Rs2
# ->
sh1add.uw Rd, Rs1, Rs2

add.uw Tmp, Rs1, X0
slli Rd, Tmp, N
# ->
slli.uw Rd, Rs1, N
```

#### Constraints

Zba and RV64 (every `zext.w`/`.uw` form is RV64-only); gated on `hasZba() && isRV64()`. Same-destination chains (the consuming op's destination equals the `zext.w` temporary), the consuming op's other source (`Rs2`) must not alias that temporary, `zext.w` is the `add.uw rd, rs, x0` alias, N within the `slli.uw` immediate field (N >= 1; a zero shift is a NOP owned by `NopRule`), and rd=x0 excluded. The deletion arm's producer table stays conservative: `lwu`, `lbu`/`lhu`, `zext.h`, `add.uw` with `X0` as the addend (`zext32(Rs1) + Rs2` is 32-bit-bounded only then), `andi` with a non-negative immediate, and `srli` by at least 32. Operations: `X(rd) = X(rs2) + zext32(X(rs1))` for `add.uw` and `X(rd) = zext32(X(rs1)) << shamt` for `slli.uw`.

#### Evidence

* <https://github.com/llvm/llvm-project/blob/37b7c17388717199e9669e3ea5bb2a5c9711bbb1/llvm/lib/Target/RISCV/RISCVInstrInfoZb.td#L754-L760>
* <https://github.com/gcc-mirror/gcc/blob/6afcc4f6da931eb93f3ab001a0dd9650ea71d1ea/gcc/config/riscv/riscv.md#L1863-L1889>

### `ZbbSextRule` (`riscv:integer/zbb-sext`)

```asm
slli Rd, Rs, XLEN-16
srai Rd, Rd, XLEN-16
# ->
sext.h Rd, Rs

slli Rd, Rs, XLEN-8
srai Rd, Rd, XLEN-8
# ->
sext.b Rd, Rs

lb Rd, Off(Rs1)
sext.b Rd, Rd
# ->
# delete

lh Rd, Off(Rs1)
sext.h Rd, Rd
# ->
# delete

lb Rd, Off(Rs1)
sext.h Rd, Rd
# ->
# delete

sext.b Rd, Rs
sext.b Rd, Rd
# ->
# delete

sext.h Rd, Rs
sext.h Rd, Rd
# ->
# delete

sext.b Rd, Rs
sext.h Rd, Rd
# ->
# delete
```

#### Constraints

Zbb; gated on `hasZbb()`. Same destination, rd=x0 excluded. The shift amounts are `XLEN-16` (48 on RV64, 16 on RV32) and `XLEN-8` (56/24). `LB` already sign-extends a byte, so a following `sext.b` or `sext.h` is a NOP; `LH` already sign-extends a halfword, so `sext.h` is a NOP. The extension arms are idempotent, and `sext.h` after `sext.b` is also redundant because a byte-extended value is already halfword-extended; `sext.b` after `sext.h` is not (it narrows the value).

#### Evidence

* <https://github.com/llvm/llvm-project/blob/37b7c17388717199e9669e3ea5bb2a5c9711bbb1/llvm/lib/Target/RISCV/RISCVInstrInfoZb.td#L603-L604>
* <https://github.com/gcc-mirror/gcc/blob/6afcc4f6da931eb93f3ab001a0dd9650ea71d1ea/gcc/config/riscv/riscv.md#L1971-L1994>

### `ZbbZextHRule` (`riscv:integer/zbb-zext-h`)

```asm
slli Rd, Rs, XLEN-16
srli Rd, Rd, XLEN-16
# ->
zext.h Rd, Rs

lbu Rd, Off(Rs1)
zext.h Rd, Rd
# or
lhu Rd, Off(Rs1)
zext.h Rd, Rd
# or
zext.h Rd, Rs
zext.h Rd, Rd
# or
andi Rd, Rs, Imm
zext.h Rd, Rd
# or
srli Rd, Rs, N
zext.h Rd, Rd
# ->
# delete
```

#### Constraints

Zbb or Zbkb; gated on `hasZbb() || hasZbkb()`. Same destination, rd=x0 excluded. The shift amounts are `XLEN-16` (48 on RV64, 16 on RV32). The deletion arm's producer must be statically known to fit in 16 bits: `lbu`, `lhu`, `zext.h` itself, `andi` with a non-negative immediate (result at most 2047), or `srli` by at least `XLEN-16`.

#### Evidence

* <https://github.com/llvm/llvm-project/blob/37b7c17388717199e9669e3ea5bb2a5c9711bbb1/llvm/lib/Target/RISCV/RISCVInstrInfoZb.td#L716-L718>
* <https://github.com/gcc-mirror/gcc/blob/6afcc4f6da931eb93f3ab001a0dd9650ea71d1ea/gcc/config/riscv/riscv.md#L1891-L1914>
* <https://github.com/gcc-mirror/gcc/blob/6afcc4f6da931eb93f3ab001a0dd9650ea71d1ea/gcc/config/riscv/bitmanip.md#L318-L318>

### `AddiPairRule` (`riscv:integer/addi-pair`)

```asm
addi Rd, Rs, Imm0
addi Rd, Rd, Imm1
# ->
addi Rd, Rs, Imm0+Imm1
# or addi Rd, Rs, 0 (mv) when the sum is 0 and Rd != Rs
# or delete when the sum is 0 and Rd == Rs

c.addi Rd, Imm0
c.addi Rd, Imm1
# or the mixed addi/c.addi forms
# ->
addi Rd, Rd, Imm0+Imm1
# or delete when the sum is 0
```

#### Constraints

Base ISA; the `c.addi` arms need Zca. Both immediates must be nonzero (a zero-immediate member is claimed by `NopRule` only when it rewrites its own source; zero-immediate members stay excluded), the second instruction must overwrite the first destination, and the combined immediate must fit the signed 12-bit immediate. A zero sum is a deletion only when the chain's source is `Rd`; otherwise the pair is a copy (`addi Rd, Rs, 0`, printed `mv`). Two `c.addi` are 4 bytes and a single `addi` is 4, so the encoding never grows.

#### Evidence

* <https://github.com/llvm/llvm-project/blob/37b7c17388717199e9669e3ea5bb2a5c9711bbb1/llvm/lib/Target/RISCV/RISCVFoldMemOffset.cpp#L9-L15>

### `LogicImmediateRule` (`riscv:integer/logic-immediate`)

```asm
ori Rd, Rs, Imm0
ori Rd, Rd, Imm1
# ->
ori Rd, Rs, Imm0|Imm1

xori Rd, Rs, Imm0
xori Rd, Rd, Imm1
# ->
xori Rd, Rs, Imm0^Imm1

andi Rd, Rs, Imm0
andi Rd, Rd, Imm1
# ->
andi Rd, Rs, Imm0&Imm1
```

#### Constraints

Base ISA. Same destination, both immediates nonzero. The OR/XOR/AND of two sign-extended 12-bit values is always a sign-extended 12-bit value, so the combined immediate is encodable without a range check. When the combined result is a NOP (`xori` with equal immediates, `andi` with both -1), the pair is deleted when the source is `Rd` and becomes `mv` otherwise.

#### Evidence

* <https://github.com/llvm/llvm-project/blob/37b7c17388717199e9669e3ea5bb2a5c9711bbb1/llvm/lib/Target/RISCV/RISCVISelLowering.cpp#L18380-L18490>

### `ShiftChainRule` (`riscv:integer/shift-chain`)

```asm
slli Rd, Rs, Shamt0
slli Rd, Rd, Shamt1
# ->
slli Rd, Rs, Shamt0+Shamt1
# same for srli; srai clamps the sum to XLEN-1
```

#### Constraints

Base ISA plus Zca compressed forms. Same destination and same direction, both amounts nonzero. Logical shifts require `Shamt0+Shamt1 < XLEN`: a sum reaching XLEN zeroes the result, which no single shift can express, so it is not reported. Arithmetic shifts clamp to `XLEN-1` (the sign-fill shift), which is exact because `srai` never changes the sign bit. The `c.slli`/`c.srli`/`c.srai` forms compose with the base forms (the C immediate is 6 bits on RV64, 5 on RV32).

#### Evidence

* <https://github.com/llvm/llvm-project/blob/37b7c17388717199e9669e3ea5bb2a5c9711bbb1/llvm/lib/CodeGen/SelectionDAG/DAGCombiner.cpp#L11063-L11080>

### `ShiftMaskRule` (`riscv:integer/shift-mask`)

```asm
slli Rd, Rs, N
srli Rd, Rd, N
# ->
andi Rd, Rs, (1 << (XLEN - N)) - 1

srli Rd, Rs, N
slli Rd, Rd, N
# ->
andi Rd, Rs, -(1 << N)
```

#### Constraints

Base ISA. Same destination, N >= 1, and the replacement mask must fit the signed 12-bit immediate (hence `N >= XLEN-11` for the left-then-right arm and `N <= 11` for the right-then-left arm). Left-then-right keeps the low `XLEN-N` bits; right-then-left clears the low N bits (the align-down idiom).

#### Evidence

* <https://github.com/gcc-mirror/gcc/blob/6afcc4f6da931eb93f3ab001a0dd9650ea71d1ea/gcc/config/riscv/riscv.md#L1863-L1889>
* <https://github.com/gcc-mirror/gcc/blob/6afcc4f6da931eb93f3ab001a0dd9650ea71d1ea/gcc/config/riscv/riscv.md#L1891-L1914>

### `NegRule` (`riscv:integer/neg`)

```asm
xori Rd, Rs, -1
addi Rd, Rd, 1
# ->
sub Rd, X0, Rs
```

#### Constraints

Base ISA. Same destination; `xori Rd, Rs, -1` is `not` and the pair is two's-complement `~x + 1 = -x`, i.e. the `neg` idiom `sub Rd, X0, Rs`. rd=x0 stays excluded.

#### Evidence

* <https://github.com/llvm/llvm-project/blob/37b7c17388717199e9669e3ea5bb2a5c9711bbb1/llvm/lib/Target/RISCV/RISCVInstrInfo.td#L1124-L1124>
* <https://github.com/gcc-mirror/gcc/blob/6afcc4f6da931eb93f3ab001a0dd9650ea71d1ea/gcc/config/riscv/riscv.md#L1617-L1620>

### `ZbaShAddRule` (`riscv:integer/zba-sh-add`)

```asm
slli Rd, Rs1, N
add Rd, Rd, Rs2
# ->
sh1add/sh2add/sh3add Rd, Rs1, Rs2
```

#### Constraints

Zba (`sh1add`/`sh2add`/`sh3add` exist on RV32 and RV64); gated on `hasZba()`. Same destination (the `add` destination equals the `slli` destination), `Rs2` must not alias `Rd` (the `slli` overwrites it before the `add` reads it), N in 1..3, rd=x0 excluded. `add` is commutative, so either operand order matches. Operation: `X(rd) = X(rs2) + (X(rs1) << N)`.

#### Evidence

* <https://github.com/llvm/llvm-project/blob/37b7c17388717199e9669e3ea5bb2a5c9711bbb1/llvm/lib/Target/RISCV/RISCVInstrInfoZb.td#L720-L745>
* <https://github.com/llvm/llvm-project/blob/37b7c17388717199e9669e3ea5bb2a5c9711bbb1/llvm/lib/Target/RISCV/RISCVISelLowering.cpp#L17324-L17362>
* <https://github.com/gcc-mirror/gcc/blob/6afcc4f6da931eb93f3ab001a0dd9650ea71d1ea/gcc/config/riscv/bitmanip.md#L32-L32>

### `ZbbAndnRule` (`riscv:integer/zbb-andn`)

```asm
xori Tmp, Rs2, -1
and Rd, Rs1, Tmp
# or and Rd, Tmp, Rs1
# ->
andn Rd, Rs1, Rs2

xori Tmp, Rs2, -1
or Rd, Rs1, Tmp
# or or Rd, Tmp, Rs1
# ->
orn Rd, Rs1, Rs2

xori Tmp, Rs2, -1
xor Rd, Rs1, Tmp
# or xor Rd, Tmp, Rs1
# ->
xnor Rd, Rs1, Rs2
```

#### Constraints

Zbb or Zbkb; gated on `hasZbb() || hasZbkb()`. The logic instruction's destination must be the `xori -1` temporary (same-destination chain), `Rs1` must not alias that temporary (the `xori` would clobber it), and `Rs2` must not be the temporary itself: the self-inverted form is the Zbs mask-materialization middle owned by `ZbsBclrRule`. `Rs2 == X0` stays allowed (`~x0` is architecturally all ones, so `andn`/`orn`/`xnor` with a zero source compute exactly the original pair), and rd=x0 is excluded. `and`/`or`/`xor` are commutative, so either operand order matches. Distinct destinations need liveness and stay deferred.

#### Evidence

* <https://github.com/llvm/llvm-project/blob/37b7c17388717199e9669e3ea5bb2a5c9711bbb1/llvm/lib/Target/RISCV/RISCVInstrInfoZb.td#L481-L488>
* <https://github.com/gcc-mirror/gcc/blob/6afcc4f6da931eb93f3ab001a0dd9650ea71d1ea/gcc/config/riscv/bitmanip.md#L268-L268>

### `ZbbRotateRule` (`riscv:integer/zbb-rotate`)

```asm
rori Rd, Rs, Shamt0
rori Rd, Rd, Shamt1
# ->
rori Rd, Rs, (Shamt0+Shamt1) mod XLEN
# or addi Rd, Rs, 0 (mv) when the sum is 0 mod XLEN
# or delete the pair when the sum is 0 mod XLEN and Rd == Rs
```

#### Constraints

Zbb or Zbkb; gated on `hasZbb() || hasZbkb()`. Same destination, both amounts nonzero, rd=x0 excluded. Both amounts are immediate rotate amounts, so the composed amount is always encodable modulo XLEN. The 3-instruction `srli`+`slli`+`or` -> `rori` formation needs both shift temporaries dead and stays deferred.

#### Evidence

* <https://github.com/llvm/llvm-project/blob/37b7c17388717199e9669e3ea5bb2a5c9711bbb1/llvm/lib/Target/RISCV/RISCVInstrInfoZb.td#L495-L499>
* <https://github.com/gcc-mirror/gcc/blob/6afcc4f6da931eb93f3ab001a0dd9650ea71d1ea/gcc/config/riscv/bitmanip.md#L372-L372>

### `ZbsBextRule` (`riscv:integer/zbs-bext`)

```asm
srli Tmp, Rs, Imm
andi Rd, Tmp, 1
# ->
bexti Rd, Rs, Imm

srl Tmp, Rs1, Rs2
andi Rd, Tmp, 1
# ->
bext Rd, Rs1, Rs2
```

#### Constraints

Zbs; gated on `hasZbs()`. The `andi` destination must be the shift temporary, `Imm` is within the shift immediate field (a zero shift is a NOP owned by `NopRule` only when it rewrites its own source; otherwise the copy is absorbed), and rd=x0 is excluded. `bext`/`bexti` extract exactly one bit: `(X(rs1) >> index) & 1`.

#### Evidence

* <https://github.com/llvm/llvm-project/blob/37b7c17388717199e9669e3ea5bb2a5c9711bbb1/llvm/lib/Target/RISCV/RISCVInstrInfoZb.td#L519-L520>
* <https://github.com/llvm/llvm-project/blob/37b7c17388717199e9669e3ea5bb2a5c9711bbb1/llvm/lib/Target/RISCV/RISCVInstrInfoZb.td#L536-L537>
* <https://github.com/gcc-mirror/gcc/blob/6afcc4f6da931eb93f3ab001a0dd9650ea71d1ea/gcc/config/riscv/bitmanip.md#L915-L915>

### `ZbsBsetRule` (`riscv:integer/zbs-bset`)

```asm
addi Tmp, X0, 1
sll Tmp, Tmp, Rs2
or Tmp, Rs1, Tmp
# ->
bset Tmp, Rs1, Rs2

addi Tmp, X0, 1
sll Tmp, Tmp, Rs2
xor Tmp, Rs1, Tmp
# ->
binv Tmp, Rs1, Rs2
```

#### Constraints

Zbs; gated on `hasZbs()`. Same-destination chain (`Tmp` throughout), the materialization may also be `c.li Tmp, 1`, `Rs1` and `Rs2` must not alias `Tmp` (the `addi`/`sll` clobber it), and Tmp must not be `x0`. `or`/`xor` are commutative, so either operand order matches. Operations: `bset = rs1 | (1 << rs2)` and `binv = rs1 ^ (1 << rs2)`. The standalone 2-instruction `li 1; sll` prefix is not reported: when the consuming `or`/`xor` follows, this rule subsumes it, and a window-2 rule has no lookahead to suppress itself.

#### Evidence

* <https://github.com/llvm/llvm-project/blob/37b7c17388717199e9669e3ea5bb2a5c9711bbb1/llvm/lib/Target/RISCV/RISCVInstrInfoZb.td#L515-L518>
* <https://github.com/gcc-mirror/gcc/blob/6afcc4f6da931eb93f3ab001a0dd9650ea71d1ea/gcc/config/riscv/bitmanip.md#L624-L624>
* <https://github.com/gcc-mirror/gcc/blob/6afcc4f6da931eb93f3ab001a0dd9650ea71d1ea/gcc/config/riscv/bitmanip.md#L635-L635>

### `ZbsBclrRule` (`riscv:integer/zbs-bclr`)

```asm
addi Tmp, X0, 1
sll Tmp, Tmp, Rs2
xori Tmp, Tmp, -1
and Tmp, Rs1, Tmp
# or and Tmp, Tmp, Rs1
# ->
bclr Tmp, Rs1, Rs2
```

#### Constraints

Zbs; gated on `hasZbs()`. Same-destination chain, the materialization may also be `c.li Tmp, 1`, `Rs1` and `Rs2` must not alias `Tmp` (the `addi`/`sll`/`xori` clobber it), and Tmp must not be `x0`. Operation: `bclr = rs1 & ~(1 << rs2)`. The `xori -1` + `and` middle inside the pattern does not match the general `ZbbAndnRule`: the self-inverted temporary (`Rs2 == Tmp`) is left to this rule. `and` is commutative, so either operand order matches.

#### Evidence

* <https://github.com/llvm/llvm-project/blob/37b7c17388717199e9669e3ea5bb2a5c9711bbb1/llvm/lib/Target/RISCV/RISCVInstrInfoZb.td#L511-L514>
* <https://github.com/gcc-mirror/gcc/blob/6afcc4f6da931eb93f3ab001a0dd9650ea71d1ea/gcc/config/riscv/bitmanip.md#L845-L845>

### `BranchToNextRule` (`riscv:control/branch-to-next`)

```asm
beq/bne/blt/bge/bltu/bgeu Rs1, Rs2, .+4
# or
c.beqz/c.bnez Rs, .+2
# or
jal X0, .+4 (j)
# or
c.j .+2
# ->
# delete
```

#### Constraints

Direct, non-linking branches only. `JAL` with `rd = x0` and `C_J` are included; `JAL` with a link register, `C_JAL`, `JALR` and other indirect forms are excluded. The target must equal `Address + Size`, resolved with `Ctx.MIA.evaluateBranch`; taken and fallthrough paths then coincide.

#### Evidence

* <https://github.com/llvm/llvm-project/blob/37b7c17388717199e9669e3ea5bb2a5c9711bbb1/llvm/lib/Target/RISCV/MCTargetDesc/RISCVMCTargetDesc.cpp#L300-L332>

### `DegenerateBranchRule` (`riscv:control/degenerate-branch`)

```asm
beq Rs, Rs, Offs
# or
bge Rs, Rs, Offs
# or
bgeu Rs, Rs, Offs
# ->
jal X0, Offs (j)

bne Rs, Rs, Offs
# or
blt Rs, Rs, Offs
# or
bltu Rs, Rs, Offs
# ->
# delete
```

#### Constraints

Base ISA. Comparing a register with itself has a constant truth value; `beqz X0`/`bnez X0` assemble to the `beq`/`bne` forms and are covered. A target equal to `Address + Size` is left to `BranchToNextRule` so the two rules never double-report. The `jal` replacement range (±1 MiB) always covers the source branch ranges (B ±4 KiB, C ±256 B). Compressed branches cannot be degenerate: `c.beqz`/`c.bnez` read x8--x15, never x0.

#### Evidence

* <https://github.com/llvm/llvm-project/blob/37b7c17388717199e9669e3ea5bb2a5c9711bbb1/llvm/lib/Target/RISCV/RISCVLateBranchOpt.cpp#L9-L14>
* <https://github.com/llvm/llvm-project/blob/37b7c17388717199e9669e3ea5bb2a5c9711bbb1/llvm/lib/Target/RISCV/RISCVInstrInfo.cpp#L1121-L1139>

### `PCBranchRule` (`riscv:control/pc-branch`)

```asm
auipc T, Hi20
jalr T, T, Lo12
# ->
jal T, Target
```

#### Constraints

Base ISA. The call form is provable from the shared destination (`jal` writes the same return address that `jalr` writes): the `jalr` base must be the `auipc` destination, the `jalr` destination must be the same register, and T must not be `x0` (an `auipc x0` writes nothing, so the address would not be PC-relative). The rule computes `Target = (PC + sign_extend(Hi20) << 12 + sign_extend(Lo12)) & ~1` from the encoded immediates (`jalr` clears bit 0) and requires it inside the `JAL` range (±1 MiB, inherently 2-byte aligned). The tail form (`jalr X0, T, Lo12`) stays deferred until a bounded dead-temporary proof exists. Relocations are never interpreted; the rule works on final bytes. When the call target lies inside the scanned region, the engine's call-target boundary splits the window, so such in-region targets are not reported.

#### Evidence

* <https://github.com/llvm/llvm-project/blob/37b7c17388717199e9669e3ea5bb2a5c9711bbb1/llvm/lib/Target/RISCV/RISCVExpandPseudoInsts.cpp#L758-L790>
* <https://github.com/gcc-mirror/gcc/blob/6afcc4f6da931eb93f3ab001a0dd9650ea71d1ea/gcc/config/riscv/riscv.md#L4071-L4083>

### `AddressLoadRule` (`riscv:memory/address-load`)

```asm
addi Rd, Rs1, Imm0
lb/lh/lw/ld/lbu/lhu/lwu Rd, Imm1(Rd)
# ->
same load Rd, Imm0+Imm1(Rs1)
```

#### Constraints

Base ISA; `ld`/`lwu` are RV64. The load destination must equal the address temporary, proving the temporary is overwritten, the combined offset must fit the signed 12-bit load offset, and rd=x0 stays excluded. `flw`/`fld` are excluded: they write an FP register, so the address temporary is never overwritten and the rewrite needs a dead-temporary proof. Distinct-destination forms stay deferred (liveness).

#### Evidence

* <https://github.com/llvm/llvm-project/blob/37b7c17388717199e9669e3ea5bb2a5c9711bbb1/llvm/lib/Target/RISCV/RISCVFoldMemOffset.cpp#L9-L15>

### `LoadZeroExtendRule` (`riscv:memory/load-extend`)

```asm
lbu Rd, Off(Rs1)
andi Rd, Rd, 255
# ->
# delete the andi
```

#### Constraints

Base ISA. The `andi` destination must be the load destination, and rd=x0 stays excluded. `LBU` already zero-extends the byte. The sign-extending-load arms belong to `SextWRule`, `ZbbSextRule`, and `ZbbZextHRule`.

#### Evidence

* <https://github.com/gcc-mirror/gcc/blob/6afcc4f6da931eb93f3ab001a0dd9650ea71d1ea/gcc/config/riscv/riscv.md#L1921-L1931>

