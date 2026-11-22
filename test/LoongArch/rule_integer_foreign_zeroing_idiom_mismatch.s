## Reject anchors, inert writes, and near-miss forms that are not provable zeros (LA32 and LA64).
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch32-unknown-linux -filetype=obj %s -o %t.32.o
# RUN: ld.lld --entry=_start %t.32.o -o %t.32.exe
# RUN: loonglint --filter '^loongarch:integer/foreign-zeroing-idiom$' %t.32.exe | FileCheck %s

# RUN: llvm-mc -triple=loongarch64-unknown-linux -filetype=obj %s -o %t.64.o
# RUN: ld.lld --entry=_start %t.64.o -o %t.64.exe
# RUN: loonglint --filter '^loongarch:integer/foreign-zeroing-idiom$' %t.64.exe | FileCheck %s

# CHECK: 0 finding(s)

.text
.globl _start
_start:
  or $t0, $zero, $zero
  ori $t0, $zero, 0
  move $t0, $zero
  nop
  andi $zero, $t1, 0
  sub.w $zero, $t1, $t1
  andi $zero, $zero, 0
  sltui $t0, $zero, -5
  slti $t0, $t1, 0
  ori $t0, $zero, 5
  addi.w $t0, $zero, 5
  nor $t0, $zero, $zero
  clz.w $t0, $zero
  div.w $t0, $zero, $t1
  mod.w $t0, $t1, $t1
  bstrins.w $t0, $zero, 15, 0
  masknez $t0, $t1, $zero
  bytepick.w $t0, $zero, $t1, 1
  alsl.w $t0, $zero, $t1, 1
  ext.w.b $t0, $t1
