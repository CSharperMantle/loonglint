## Reject non-fusable ADDI+SUB pairs.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64-unknown-linux -filetype=obj %s -o %t.o
# RUN: ld.lld --entry=_start %t.o -o %t.exe
# RUN: loonglint %t.exe | FileCheck %s

# CHECK: 0 finding(s)

## Reject an aliased base (the SUB would read the overwritten base), a $zero
## destination (the ADDI write is discarded), distinct destinations, a
## different SUB base, a SUB that does not consume the ADDI result, and the
## unsound ADDI.W + SUB.D width mix.
.text
.globl _start
_start:
  addi.d $t0, $t0, 16
  sub.d  $t0, $t0, $t0
  addi.d $zero, $a0, 16
  sub.d  $zero, $a0, $zero
  addi.d $t1, $a0, 16
  sub.d  $t2, $a0, $t1
  addi.d $t3, $a0, 16
  sub.d  $t3, $a1, $t3
  addi.d $t4, $a0, 16
  sub.d  $t4, $t4, $t5
  addi.w $t5, $a0, 16
  sub.d  $t5, $a0, $t5
