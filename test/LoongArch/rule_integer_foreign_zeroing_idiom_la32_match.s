## Match every LA32-legal foreign zeroing idiom against the canonical 'move' replacement (LA32).
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch32-unknown-linux -filetype=obj %s -o %t.o
# RUN: ld.lld --entry=_start %t.o -o %t.exe
# RUN: not loonglint --filter '^loongarch:integer/foreign-zeroing-idiom$' %t.exe | FileCheck %s

# CHECK: sub.w $t0, $t1, $t1
# CHECK: xor $t0, $t1, $t1
# CHECK: slt $t0, $t1, $t1
# CHECK: sltu $t0, $t1, $t1
# CHECK: andi $t0, $t1, 0
# CHECK: andi $t0, $zero, 5
# CHECK: and $t0, $zero, $t1
# CHECK: and $t0, $t1, $zero
# CHECK: andn $t0, $zero, $t1
# CHECK: sll.w $t0, $zero, $t2
# CHECK: srl.w $t0, $zero, $t2
# CHECK: sra.w $t0, $zero, $t2
# CHECK: rotr.w $t0, $zero, $t2
# CHECK: slli.w $t0, $zero, 3
# CHECK: srli.w $t0, $zero, 3
# CHECK: srai.w $t0, $zero, 3
# CHECK: rotri.w $t0, $zero, 3
# CHECK: lu12i.w $t0, 0
# CHECK: addi.w $t0, $zero, 0
# CHECK: add.w $t0, $zero, $zero
# CHECK: mul.w $t0, $zero, $t1
# CHECK: mulh.w $t0, $t1, $zero
# CHECK: mulh.wu $t0, $t1, $zero
# CHECK: maskeqz $t0, $t1, $zero
# CHECK: maskeqz $t0, $zero, $t1
# CHECK: masknez $t0, $zero, $t1
# CHECK: ext.w.b $t0, $zero
# CHECK: ext.w.h $t0, $zero
# CHECK: clo.w $t0, $zero
# CHECK: cto.w $t0, $zero
# CHECK: revb.2h $t0, $zero
# CHECK: bitrev.4b $t0, $zero
# CHECK: bitrev.w $t0, $zero
# CHECK: bstrpick.w $t0, $zero, 7, 3
# CHECK: bstrins.w $t0, $zero, 31, 0
# CHECK: bytepick.w $t0, $t1, $zero, 0
# CHECK: bytepick.w $t0, $zero, $zero, 2
# CHECK: sltui $t0, $t1, 0
# CHECK: slti $t0, $zero, -5
# CHECK: slti $t0, $zero, 0
# CHECK: move $t0, $zero
# CHECK: 40 finding(s)
# CHECK: 40 loongarch:integer/foreign-zeroing-idiom

.text
.globl _start
_start:
  sub.w $t0, $t1, $t1
  xor $t0, $t1, $t1
  slt $t0, $t1, $t1
  sltu $t0, $t1, $t1
  andi $t0, $t1, 0
  andi $t0, $zero, 5
  and $t0, $zero, $t1
  and $t0, $t1, $zero
  andn $t0, $zero, $t1
  sll.w $t0, $zero, $t2
  srl.w $t0, $zero, $t2
  sra.w $t0, $zero, $t2
  rotr.w $t0, $zero, $t2
  slli.w $t0, $zero, 3
  srli.w $t0, $zero, 3
  srai.w $t0, $zero, 3
  rotri.w $t0, $zero, 3
  lu12i.w $t0, 0
  addi.w $t0, $zero, 0
  add.w $t0, $zero, $zero
  mul.w $t0, $zero, $t1
  mulh.w $t0, $t1, $zero
  mulh.wu $t0, $t1, $zero
  maskeqz $t0, $t1, $zero
  maskeqz $t0, $zero, $t1
  masknez $t0, $zero, $t1
  ext.w.b $t0, $zero
  ext.w.h $t0, $zero
  clo.w $t0, $zero
  cto.w $t0, $zero
  revb.2h $t0, $zero
  bitrev.4b $t0, $zero
  bitrev.w $t0, $zero
  bstrpick.w $t0, $zero, 7, 3
  bstrins.w $t0, $zero, 31, 0
  bytepick.w $t0, $t1, $zero, 0
  bytepick.w $t0, $zero, $zero, 2
  sltui $t0, $t1, 0
  slti $t0, $zero, -5
  slti $t0, $zero, 0
