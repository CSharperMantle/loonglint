## Reject non-fusable ADDI.W+SUB.W pairs on LA32.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch32-unknown-linux -filetype=obj %s -o %t.o
# RUN: ld.lld --entry=_start %t.o -o %t.exe
# RUN: loonglint %t.exe | FileCheck %s

# CHECK: 0 finding(s)

## Reject an aliased base, a $zero destination, distinct destinations, a
## different SUB base, and a SUB that does not consume the ADDI result.
.text
.globl _start
_start:
  addi.w $t0, $t0, 16
  sub.w  $t0, $t0, $t0
  addi.w $zero, $a0, 16
  sub.w  $zero, $a0, $zero
  addi.w $t1, $a0, 16
  sub.w  $t2, $a0, $t1
  addi.w $t3, $a0, 16
  sub.w  $t3, $a1, $t3
  addi.w $t4, $a0, 16
  sub.w  $t4, $t4, $t5
