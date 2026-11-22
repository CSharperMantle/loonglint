## Match every operand-provably-zero idiom against the canonical 'move' replacement (LA64).
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64-unknown-linux -filetype=obj %s -o %t.o
# RUN: ld.lld --entry=_start %t.o -o %t.exe
# RUN: not loonglint --filter '^loongarch:integer/foreign-zeroing-idiom$' %t.exe | FileCheck %s

# CHECK: sub.w $t0, $t1, $t1
# CHECK: add.d $t0, $zero, $zero
# CHECK: sub.d $t0, $t1, $t1
# CHECK: addi.w $t0, $zero, 0
# CHECK: addi.d $t0, $zero, 0
# CHECK: addu16i.d $t0, $zero, 0
# CHECK: alsl.w $t0, $zero, $zero, 1
# CHECK: alsl.wu $t0, $zero, $zero, 2
# CHECK: alsl.d $t0, $zero, $zero, 4
# CHECK: lu12i.w $t0, 0
# CHECK: lu52i.d $t0, $zero, 0
# CHECK: slt $t0, $t1, $t1
# CHECK: sltu $t0, $t1, $t1
# CHECK: slti $t0, $zero, -5
# CHECK: slti $t0, $zero, 0
# CHECK: sltui $t0, $t1, 0
# CHECK: and $t0, $zero, $t1
# CHECK: and $t0, $t1, $zero
# CHECK: xor $t0, $t1, $t1
# CHECK: andn $t0, $zero, $t1
# CHECK: andi $t0, $t1, 0
# CHECK: andi $t0, $zero, 5
# CHECK: mul.w $t0, $zero, $t1
# CHECK: mulh.w $t0, $t1, $zero
# CHECK: mulh.wu $t0, $t1, $zero
# CHECK: mul.d $t0, $zero, $t1
# CHECK: mulh.d $t0, $t1, $zero
# CHECK: mulh.du $t0, $t1, $zero
# CHECK: mulw.d.w $t0, $zero, $t1
# CHECK: mulw.d.wu $t0, $zero, $t1
# CHECK: sll.w $t0, $zero, $t2
# CHECK: srl.w $t0, $zero, $t2
# CHECK: sra.w $t0, $zero, $t2
# CHECK: rotr.w $t0, $zero, $t2
# CHECK: slli.w $t0, $zero, 3
# CHECK: srli.w $t0, $zero, 3
# CHECK: srai.w $t0, $zero, 3
# CHECK: rotri.w $t0, $zero, 3
# CHECK: sll.d $t0, $zero, $t2
# CHECK: srl.d $t0, $zero, $t2
# CHECK: sra.d $t0, $zero, $t2
# CHECK: rotr.d $t0, $zero, $t2
# CHECK: slli.d $t0, $zero, 3
# CHECK: srli.d $t0, $zero, 3
# CHECK: srai.d $t0, $zero, 3
# CHECK: rotri.d $t0, $zero, 3
# CHECK: ext.w.b $t0, $zero
# CHECK: ext.w.h $t0, $zero
# CHECK: clo.w $t0, $zero
# CHECK: cto.w $t0, $zero
# CHECK: clo.d $t0, $zero
# CHECK: cto.d $t0, $zero
# CHECK: bytepick.w $t0, $t1, $zero, 0
# CHECK: bytepick.d $t0, $zero, $zero, 5
# CHECK: revb.2h $t0, $zero
# CHECK: revb.4h $t0, $zero
# CHECK: revb.2w $t0, $zero
# CHECK: revb.d $t0, $zero
# CHECK: revh.2w $t0, $zero
# CHECK: revh.d $t0, $zero
# CHECK: bitrev.4b $t0, $zero
# CHECK: bitrev.8b $t0, $zero
# CHECK: bitrev.w $t0, $zero
# CHECK: bitrev.d $t0, $zero
# CHECK: bstrins.w $t0, $zero, 31, 0
# CHECK: bstrins.d $t0, $zero, 63, 0
# CHECK: bstrpick.w $t0, $zero, 7, 3
# CHECK: bstrpick.d $t0, $zero, 63, 0
# CHECK: maskeqz $t0, $t1, $zero
# CHECK: maskeqz $t0, $zero, $t1
# CHECK: masknez $t0, $zero, $t1
# CHECK: move $t0, $zero
# CHECK: 72 finding(s)
# CHECK: 72 loongarch:integer/foreign-zeroing-idiom

.text
.globl _start
_start:
  add.w $t0, $zero, $zero
  sub.w $t0, $t1, $t1
  add.d $t0, $zero, $zero
  sub.d $t0, $t1, $t1
  addi.w $t0, $zero, 0
  addi.d $t0, $zero, 0
  addu16i.d $t0, $zero, 0
  alsl.w $t0, $zero, $zero, 1
  alsl.wu $t0, $zero, $zero, 2
  alsl.d $t0, $zero, $zero, 4
  lu12i.w $t0, 0
  lu52i.d $t0, $zero, 0
  slt $t0, $t1, $t1
  sltu $t0, $t1, $t1
  slti $t0, $zero, -5
  slti $t0, $zero, 0
  sltui $t0, $t1, 0
  and $t0, $zero, $t1
  and $t0, $t1, $zero
  xor $t0, $t1, $t1
  andn $t0, $zero, $t1
  andi $t0, $t1, 0
  andi $t0, $zero, 5
  mul.w $t0, $zero, $t1
  mulh.w $t0, $t1, $zero
  mulh.wu $t0, $t1, $zero
  mul.d $t0, $zero, $t1
  mulh.d $t0, $t1, $zero
  mulh.du $t0, $t1, $zero
  mulw.d.w $t0, $zero, $t1
  mulw.d.wu $t0, $zero, $t1
  sll.w $t0, $zero, $t2
  srl.w $t0, $zero, $t2
  sra.w $t0, $zero, $t2
  rotr.w $t0, $zero, $t2
  slli.w $t0, $zero, 3
  srli.w $t0, $zero, 3
  srai.w $t0, $zero, 3
  rotri.w $t0, $zero, 3
  sll.d $t0, $zero, $t2
  srl.d $t0, $zero, $t2
  sra.d $t0, $zero, $t2
  rotr.d $t0, $zero, $t2
  slli.d $t0, $zero, 3
  srli.d $t0, $zero, 3
  srai.d $t0, $zero, 3
  rotri.d $t0, $zero, 3
  ext.w.b $t0, $zero
  ext.w.h $t0, $zero
  clo.w $t0, $zero
  cto.w $t0, $zero
  clo.d $t0, $zero
  cto.d $t0, $zero
  bytepick.w $t0, $t1, $zero, 0
  bytepick.d $t0, $zero, $zero, 5
  revb.2h $t0, $zero
  revb.4h $t0, $zero
  revb.2w $t0, $zero
  revb.d $t0, $zero
  revh.2w $t0, $zero
  revh.d $t0, $zero
  bitrev.4b $t0, $zero
  bitrev.8b $t0, $zero
  bitrev.w $t0, $zero
  bitrev.d $t0, $zero
  bstrins.w $t0, $zero, 31, 0
  bstrins.d $t0, $zero, 63, 0
  bstrpick.w $t0, $zero, 7, 3
  bstrpick.d $t0, $zero, 63, 0
  maskeqz $t0, $t1, $zero
  maskeqz $t0, $zero, $t1
  masknez $t0, $zero, $t1
