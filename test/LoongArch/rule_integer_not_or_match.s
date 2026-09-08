## Match OR-NOT pairs folding to NOR.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch32-unknown-linux -filetype=obj %s -o %t.32.o
# RUN: ld.lld --entry=_start %t.32.o -o %t.32.exe
# RUN: not loonglint %t.32.exe | FileCheck %s

# RUN: llvm-mc -triple=loongarch64-unknown-linux -filetype=obj %s -o %t.64.o
# RUN: ld.lld --entry=_start %t.64.o -o %t.64.exe
# RUN: not loonglint %t.64.exe | FileCheck %s

## Covers both OR operand orders and both NOR $zero operand orders.
# CHECK-COUNT-4: [integer/not-or]
# CHECK: 4 finding(s)
# CHECK: 4 integer/not-or

.text
.globl _start
_start:
  or   $t0, $a0, $a1
  nor  $t0, $t0, $zero
  or   $t1, $a2, $a3
  nor  $t1, $zero, $t1
  or   $t2, $a5, $a4
  nor  $t2, $t2, $zero
  or   $t3, $a7, $a6
  nor  $t3, $zero, $t3
