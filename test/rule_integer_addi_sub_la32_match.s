## Fold common-base ADDI.W+SUB.W pairs into a constant on LA32.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch32-unknown-linux -filetype=obj %s -o %t.o
# RUN: ld.lld --entry=_start %t.o -o %t.exe
# RUN: not loonglint %t.exe | FileCheck %s

## Covers both SUB operand orders, a $zero base, and the -2048 immediate
## bound that needs ORI.
# CHECK-COUNT-4: [integer/addi-sub]
# CHECK: 4 finding(s)
# CHECK: 4 integer/addi-sub

.text
.globl _start
_start:
  addi.w $t0, $a0, 16
  sub.w  $t0, $a0, $t0
  addi.w $t1, $a1, -5
  sub.w  $t1, $t1, $a1
  addi.w $t2, $a2, -2048
  sub.w  $t2, $a2, $t2
  addi.w $t3, $zero, 16
  sub.w  $t3, $zero, $t3
