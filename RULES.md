# LoongLint implemented rules

This documents peephole rules currently shipped in LoongLint, in `RuleManager` registration order.

## `NopRule` (`integer/nop`)

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

### Constraints

LA32/LA64. Width-independent logical forms.

* Every form writes the same value back to the same register with no side effects, and
* Width-independent because the base logical ops and logical-immediates do not sign-extend.
* The canonical NOP (`ANDI $zero, $zero, 0`) is intentionally not reported. NOPs are intentional padding with architectural special semantics.

`x | 0 = x`, `x | x = x`, `x & x = x`, `x & ~0 = x`, `x ^ 0 = x` at the native register width.

### Evidence

* <https://github.com/gcc-mirror/gcc/blob/6afcc4f6da931eb93f3ab001a0dd9650ea71d1ea/gcc/config/loongarch/loongarch.md#L2659-L2659>
* LoongArch Reference Manual Volume 1, §2.2.1.10.

## `NopLA32Rule` (`integer/nop-la32`)

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

### Constraints

LA32 only.

* Do not enable on LA64: `.W` operations sign-extend their 32-bit result on LA64, so these are identities only at the native 32-bit width.

`x + 0 = x`, `x - 0 = x`, `x << 0 = x`, `x >> 0 = x`, `x rotate 0 = x` at 32-bit width.

### Evidence

* <https://github.com/gcc-mirror/gcc/blob/6afcc4f6da931eb93f3ab001a0dd9650ea71d1ea/gcc/config/loongarch/loongarch.md#L732-L732>

## `NopLA64Rule` (`integer/nop-la64`)

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

### Constraints

LA64 only.

* `.D` operations do not sign-extend, so all listed forms are exact identities at 64-bit width.

`x + 0 = x`, `x - 0 = x`, `x << 0 = x`, `x >> 0 = x`, `x rotate 0 = x` at 64-bit width.

### Evidence

* <https://github.com/gcc-mirror/gcc/blob/6afcc4f6da931eb93f3ab001a0dd9650ea71d1ea/gcc/config/loongarch/loongarch.md#L790-L790>

## `BitExtractRule` (`integer/bit-extract`)

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

### Constraints

`BSTRPICK.D` on LA64; `BSTRPICK.W` on LA32/LA64.

* `Mask = (1 << Len) - 1` must be a nonzero contiguous low-bit mask fitting `ANDI`'s 12-bit unsigned immediate, and
* In shift-first order `Msb = Lsb + Len - 1` and `Lsb + Len` must not exceed the selected width, and
* In mask-first order `Msb = Len - 1` and `Len` must exceed `Lsb`, and
* `Lsb` must be at least 1 so the pair never overlaps an identity.

Shift-first extracts `Rj[Lsb+Len-1:Lsb]`; mask-first extracts `Rj[Len-1:Lsb]` because the AND has already cleared every bit above the field. Both forms extract the same low field that `BSTRPICK` extracts directly, with width-specific sign/zero extension.

### Evidence

* <https://github.com/llvm/llvm-project/blob/37b7c17388717199e9669e3ea5bb2a5c9711bbb1/llvm/lib/Target/LoongArch/LoongArchISelLowering.cpp#L6362-L6413>

## `ZeroExtendRule` (`integer/zero-extend`)

```asm
slli.[wd] Rd, Rj, Shamt
srli.[wd] Rd, Rd, Shamt
# ->
bstrpick.[wd] Rd, Rj, Msb, 0

# where Msb = width - Shamt - 1
```

### Constraints

`BSTRPICK.D` on LA64; `BSTRPICK.W` on LA32/LA64.

* `Shamt` must be in `[1, width - 1]`, and
* The second shift must reuse the first destination with the same amount.

A logical left shift then logical right shift by the same amount clears the high `width - Shamt` bits, which `BSTRPICK` with `msb = width - Shamt - 1`, `lsb = 0` does directly as a zero-extension.

### Evidence

* <https://github.com/llvm/llvm-project/blob/37b7c17388717199e9669e3ea5bb2a5c9711bbb1/llvm/lib/Target/LoongArch/LoongArchISelLowering.cpp#L6380-L6413>

## `BitReverseRule` (`integer/bit-reverse`)

```asm
revb.2w Rd, Rj
bitrev.w Rd, Rd
# ->
bitrev.4b Rd, Rj

# either instruction order is accepted; the LA64 doubleword form is:
revb.d + bitrev.d  ->  bitrev.8b   (either order)
```

### Constraints

LA64 only.

* The second instruction must overwrite the first's destination, and
* Keep `.w`/`.d` and `.4b`/`.8b` widths paired.

A byte-within-word reversal composed with a whole-word bit reversal leaves bit reversal within each byte.

### Evidence

* <https://github.com/llvm/llvm-project/blob/37b7c17388717199e9669e3ea5bb2a5c9711bbb1/llvm/lib/Target/LoongArch/LoongArchInstrInfo.td#L1919-L1925>

## `ByteReverseRule` (`integer/byte-reverse`)

```asm
revb.4h Rd, Rj
revh.d Rd, Rd
# ->
revb.d Rd, Rj

# either instruction order is accepted.
```

### Constraints

LA64 only.

* The second instruction must overwrite the first's destination.

A byte-within-halfword reversal composed with a halfword-order reversal is a whole-doubleword byte reversal, which `REVB.D` performs directly.

### Evidence

* <https://github.com/llvm/llvm-project/blob/37b7c17388717199e9669e3ea5bb2a5c9711bbb1/llvm/lib/Target/LoongArch/LoongArchInstrInfo.td#L1919-L1925>

## `MulhSextRule` (`integer/mulh-sext`)

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

### Constraints

LA64 only; on LA32 the same-destination extension is an identity handled by `NopLA32Rule`.

* Match only same-destination word sign-extension idioms (`ADDI.W Rd, Rd, 0` or `SLLI.W Rd, Rd, 0`) after either `MULH.W` or `MULH.WU`.

`MULH.W` and `MULH.WU` already sign-extend their 32-bit high result to the GPR width on LA64, so the following same-destination extension is redundant.

### Evidence

* <https://github.com/gcc-mirror/gcc/blob/6afcc4f6da931eb93f3ab001a0dd9650ea71d1ea/gcc/config/loongarch/loongarch.md#L986-L1001>

## `ShiftChainRule` (`integer/shift-chain`)

```asm
slli.d Rd, Rj, Shamt0
slli.d Rd, Rd, Shamt1
# ->
slli.d Rd, Rj, Shamt

# srli.[wd] compose the same way; srai.[wd] clamps the combined amount
# to the width maximum instead of rejecting overflow. Word forms
# substitute .w for .d and 31 for 63.
```

### Constraints

`.D` on LA64; `.W` on LA32/LA64.

* the second destination must overwrite the first, and
* the combined amount must stay in the replacement's legal range (clamped for arithmetic right shifts), and
* both amounts must be nonzero so the pair never overlaps an identity.

Fixed-width immediate shifts compose by adding amounts; arithmetic right shifts saturate at the width maximum.

### Evidence

* <https://github.com/llvm/llvm-project/blob/37b7c17388717199e9669e3ea5bb2a5c9711bbb1/llvm/lib/Target/LoongArch/LoongArchInstrInfo.td#L1250-L1264>

## `ShiftDoubleRule` (`integer/shift-self-add`)

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

### Constraints

`.D` on LA64; `.W` on LA32/LA64.

* `ShamtPlusOne = Shamt + 1` must be encodable (`Shamt <= 62` for `.D`, `<= 30` for `.W`), and
* the same destination in both instructions keeps the temporary dead without a liveness proof.

Doubling a fixed-width shifted value equals shifting by one additional bit at that width.

### Evidence

* <https://github.com/llvm/llvm-project/blob/37b7c17388717199e9669e3ea5bb2a5c9711bbb1/llvm/lib/Target/LoongArch/LoongArchInstrInfo.td#L652-L672>

## `RotateCombineRule` (`integer/rotate-combine`)

```asm
rotri.d Rd, Rj, Shamt0
rotri.d Rd, Rd, Shamt1
# ->
rotri.d Rd, Rj, Shamt

# word forms substitute .w for .d and 32 for 64;
# Shamt = (Shamt0 + Shamt1) modulo width
```

### Constraints

`.D` on LA64; `.W` on LA32/LA64.

* Both destinations must be the same register, and
* Both amounts must be nonzero so the pair never overlaps an identity.

Immediate rotations compose modulo the operand width.

### Evidence

* <https://github.com/llvm/llvm-project/blob/37b7c17388717199e9669e3ea5bb2a5c9711bbb1/llvm/lib/Target/LoongArch/LoongArchInstrInfo.td#L1290-L1292>

## `ShiftMaskRule` (`integer/shift-mask`)

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

### Constraints

`.W` on LA32/LA64; `.D` on LA64.

* The mask destination must be overwritten by the shift, and
* `ValueRj` must not alias `Rd`, or removing the mask would change the shifted value, and
* An `ANDI` count mask must keep every count bit, and
* A `BSTRPICK.D` count producer must have `lsb` 0 and keep at least the count bits (`Msb + 1 >= log2(Width)`).

LoongArch variable shifts already consume only the low 5 (`.W`) or 6 (`.D`) shift-count bits, so any count masking that preserves those bits is redundant.

### Evidence

* <https://github.com/llvm/llvm-project/blob/37b7c17388717199e9669e3ea5bb2a5c9711bbb1/llvm/lib/Target/LoongArch/LoongArchISelDAGToDAG.cpp#L283-L340>

## `AndNotRule` (`integer/and-not`)

```asm
nor Rd, NotRk, $zero
and Rd, AndRj, Rd
# ->
andn Rd, AndRj, NotRk

# all four operand-order combinations of the NOR $zero operand and
# the AND temp operand are accepted.
```

### Constraints

LA32/LA64.

* The `AND` must overwrite the `NOR` temporary, and
* Neither `NotRk` nor `AndRj` may alias that temporary, and
* Both `NOR`/`AND` operand orders are handled.

`ANDN` is precisely `rj & ~rk`.

### Evidence

* <https://github.com/llvm/llvm-project/blob/37b7c17388717199e9669e3ea5bb2a5c9711bbb1/llvm/lib/Target/LoongArch/LoongArchInstrInfo.td#L1421-L1421>

## `OrNotRule` (`integer/or-not`)

```asm
nor Rd, NotRk, $zero
or Rd, OrRj, Rd
# ->
orn Rd, OrRj, NotRk

# all four operand-order combinations are accepted.
```

### Constraints

LA32/LA64.

* The `OR` must overwrite the `NOR` temporary, and
* Neither `NotRk` nor `OrRj` may alias that temporary, and
* Both `NOR`/`OR` operand orders are handled.

`ORN` is precisely `rj | ~rk`.

### Evidence

* <https://github.com/llvm/llvm-project/blob/37b7c17388717199e9669e3ea5bb2a5c9711bbb1/llvm/lib/Target/LoongArch/LoongArchInstrInfo.td#L1420-L1420>

## `NotOrRule` (`integer/not-or`)

```asm
or Rd, Rj, Rk
nor Rd, Rd, $zero
# ->
nor Rd, Rj, Rk

# both OR operand orders and both NOR $zero operand orders are accepted.
```

### Constraints

LA32/LA64.

* The `NOR` must overwrite the `OR` temporary, and
* `OR` operand order is interchangeable.

`NOR` is the one-instruction form of `NOT(OR(...))`.

### Evidence

* <https://github.com/llvm/llvm-project/blob/37b7c17388717199e9669e3ea5bb2a5c9711bbb1/llvm/lib/Target/LoongArch/LoongArchInstrInfo.td#L1419-L1419>

## `BitCountRule` (`integer/bit-count`)

```asm
nor NotRd, Rj, $zero
clz.w Rd, NotRd
# ->
clo.w Rd, Rj

# ctz.w -> cto.w; the .d forms substitute .d for .w.
# the NOR $zero operand may also be nor NotRd, $zero, Rj.
```

### Constraints

`.W` on LA32/LA64; `.D` on LA64.

* The count instruction must overwrite the `NOR` temporary, and
* Do not infer a complement through an arbitrary intervening instruction.

Leading/trailing zero counts of `~x` equal leading/trailing one counts of `x` at the same width.

### Evidence

* <https://github.com/llvm/llvm-project/blob/37b7c17388717199e9669e3ea5bb2a5c9711bbb1/llvm/lib/Target/LoongArch/LoongArchInstrInfo.td#L1451-L1458>

## `BranchToNextRule` (`control/branch-to-next`)

```asm
b 4
# or any non-linking direct conditional branch to PC + 4
# ->
# delete
```

### Constraints

LA32/LA64.

* The branch must be non-linking and direct, and
* Its resolved target must be the immediately following instruction, and
* Calls and indirect branches are excluded.

A branch to PC + 4 changes neither architectural state nor control flow.

### Evidence

* <https://github.com/llvm/llvm-project/blob/37b7c17388717199e9669e3ea5bb2a5c9711bbb1/llvm/include/llvm/MC/MCInstrAnalysis.h#L187>

## `DegenerateBranchRule` (`control/degenerate-branch`)

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

### Constraints

LA32/LA64.

* Preserve direct target and branch range, and
* A target equal to PC + 4 is left to `BranchToNextRule` so the two rules never double-report.

Integer comparisons of a register with itself, or zero with zero, have constant truth values.

### Evidence

* <https://github.com/llvm/llvm-project/blob/37b7c17388717199e9669e3ea5bb2a5c9711bbb1/llvm/include/llvm/MC/MCInstrAnalysis.h#L57-L82>

## `ShiftAddAlslDRule` (`integer/shift-add-alsl-d`)

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

### Constraints

LA64 only.

* `Shamt` is 1--4, and
* `SlliRd` is not `$zero`, and
* `AlslRk` is the non-temporary `ADD.D` source and must not alias `SlliRd`.
* The producer temporary is consumed by the overwriting `ADD.D`.
* Both `ADD.D` operand orders are accepted.

`ALSL.D` reads both original sources before writing. It is not equivalent when the second `ADD.D` source aliases the shifted temporary.

### Evidence

* <https://github.com/llvm/llvm-project/blob/37b7c17388717199e9669e3ea5bb2a5c9711bbb1/llvm/lib/Target/LoongArch/LoongArchInstrInfo.td#L1480-L1490>

## `AddiPairRule` (`integer/addi-pair`)

```asm
addi.w Rd, Rj, Imm0
addi.w Rd, Rd, Imm1
# ->
addi.w Rd, Rj, Combined

# on LA64, substitute addi.d for addi.w.
```

### Constraints

`.W` on LA32/LA64; `.D` on LA64.

* the second destination must overwrite the first, and
* both immediates must be nonzero (an `ADDI`-by-0 is an identity, not a pair member), and
* the combined immediate must fit the replacement's signed 12-bit range.

Fixed-width addition is associative modulo the operation width; no memory or control state is involved.

### Evidence

* <https://github.com/llvm/llvm-project/blob/37b7c17388717199e9669e3ea5bb2a5c9711bbb1/llvm/lib/Target/LoongArch/LoongArchInstrInfo.td#L1318-L1330>

## `AddressLoadRule` (`memory/address-load`)

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

### Constraints

`ADDI.D` address arithmetic with all listed loads on LA64; `ADDI.W` address arithmetic with `LD.{B,H,W,BU,HU}` on LA32.

* The load data width is independent of the address-calculation width.
* For ordinary loads, `CombinedOffset = AddSi12 + LoadOffset` must fit the signed 12-bit byte offset.
* For `LDPTR.{W/D}`, `CombinedOffset = AddSi12 + LoadOffset` must be a signed 14-bit value shifted left by 2.
* The address temporary must not be `$zero`.
* The load destination must equal the address temporary, proving that the temporary is overwritten.

For `LDPTR.{W/D}`, decoded `MCOperand` immediates are byte offsets even though the encoded `si14` field is shifted left by 2; validate the combined byte offset with the shifted-14-bit constraint and keep that byte offset in the replacement `MCInst`.

`ADDI.W` on LA64 is intentionally excluded: without an address/sign-extension proof, it is not interchangeable with the 64-bit address calculation.

One base-register addition plus one immediate-addressed load folds into the load displacement when the effective address and destination aliasing are preserved. Distinct-destination forms remain deferred because they require liveness analysis.

### Evidence

* <https://github.com/llvm/llvm-project/blob/37b7c17388717199e9669e3ea5bb2a5c9711bbb1/llvm/lib/Target/LoongArch/LoongArchInstrInfo.cpp#L917-L998>

## `LoadExtendRule` (`memory/load-extend`)

```asm
ld.b Rd, Rj, Si12
ext.w.b Rd, Rd
# ->
ld.b Rd, Rj, Si12   # delete the extension

# or ld.h + ext.w.h (LA32/LA64); ld.w + addi.w Rd, Rd, 0 or
# slli.w Rd, Rd, 0 (LA64 only); or ldptr.w + addi.w/slli.w
# Rd, Rd, 0 (LA64 only).
```

### Constraints

`EXT.W.B`/`EXT.W.H` on LA32/LA64; the `LD.W` and `LDPTR.W` forms on LA64 only (on LA32 the `ADDI.W`/`SLLI.W`-by-0 identities are handled by `NopLA32Rule`).

* The extension destination must be the load destination.

The signed loads and `LDPTR.W` already perform the requested sign extension.

### Evidence

* <https://github.com/llvm/llvm-project/blob/37b7c17388717199e9669e3ea5bb2a5c9711bbb1/llvm/lib/Target/LoongArch/LoongArchInstrInfo.td#L1949-L1953>

## `IndexedLoadRule` (`memory/indexed-load`)

```asm
add.d Rd, Rj, Rk
ld.* Rd, Rd, 0
# ->
ldx.* Rd, Rj, Rk
```

### Constraints

LA64 only.

* The address temporary must not be `$zero`, and
* The address sources `Rj`/`Rk` must not alias `Rd`, since the indexed load reads them after the `ADD.D` has written the temporary.

One base-plus-index addition followed by a zero-offset load folds into the indexed load when the index sources are preserved.

### Evidence

* <https://github.com/llvm/llvm-project/blob/37b7c17388717199e9669e3ea5bb2a5c9711bbb1/llvm/lib/Target/LoongArch/LoongArchInstrInfo.td#L1969-L1978>

## `LoadZeroExtendRule` (`memory/load-zero-extend`)

```asm
ld.[bhw] Rd, Rj, Si12
bstrpick.d Rd, Rd, {7,15,31}, 0
# ->
ld.{bu,hu,wu} Rd, Rj, Si12   # LA64

# on LA32, ld.[bh] + bstrpick.w Rd, Rd, {7,15}, 0 -> ld.{bu,hu}
```

### Constraints

`BSTRPICK.D` on LA64; `BSTRPICK.W` on LA32 (`BSTRPICK.W` sign-extends on LA64 and would not zero-extend there).

* The `BSTRPICK` must extract the full loaded low field (`msb` of 7/15/31 with `lsb = 0`) and overwrite the load destination.

Extracting the loaded low field with `BSTRPICK` is exactly what the unsigned loads `LD.BU`/`LD.HU`/`LD.WU` produce.

### Evidence

* <https://github.com/llvm/llvm-project/blob/37b7c17388717199e9669e3ea5bb2a5c9711bbb1/llvm/lib/Target/LoongArch/LoongArchInstrInfo.td#L1954-L1959>

## `UnsignedLoadPickRule` (`memory/unsigned-load-pick`)

```asm
ld.bu Rd, Rj, Si12
bstrpick.d Rd, Rd, 7, 0  # LA64
# ->
ld.bu Rd, Rj, Si12

# On LA32 the pick is bstrpick.w Rd, Rd, {7,15}, 0 after ld.{bu,hu}.
```

### Constraints

`BSTRPICK.D` picks on LA64; `BSTRPICK.W` picks on LA32. On LA32 `BSTRPICK.W` zero-extends the field across the whole 32-bit GRLEN.

* The `BSTRPICK` must extract exactly the loaded width (`msb` of 7/15/31 with `lsb = 0`) and overwrite the load destination.

An unsigned load already produces the zero-extended field, so a full-width `BSTRPICK` of the same field rewrites the same value and can be deleted.
