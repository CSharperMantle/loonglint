## Fold common-base ADDI+SUB pairs into a constant.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64-unknown-linux -filetype=obj %s -o %t.o
# RUN: ld.lld --entry=_start %t.o -o %t.exe
# RUN: not loonglint %t.exe | FileCheck %s

## Covers both SUB operand orders, a $zero base, .W pairs on LA64, the mixed
## ADDI.D + SUB.W pair, and the -2048 immediate bound that needs ORI.
# CHECK-COUNT-6: [integer/addi-sub]
# CHECK: 6 finding(s)
# CHECK: 6 integer/addi-sub

.text
.globl _start
_start:
  addi.d $t0, $a0, 16
  sub.d  $t0, $a0, $t0
  addi.d $t1, $a1, -5
  sub.d  $t1, $t1, $a1
  addi.d $t2, $a2, -2048
  sub.d  $t2, $a2, $t2
  addi.w $t3, $a3, 16
  sub.w  $t3, $a3, $t3
  addi.d $t4, $a4, 16
  sub.w  $t4, $a4, $t4
  addi.d $t5, $zero, 16
  sub.d  $t5, $zero, $t5
