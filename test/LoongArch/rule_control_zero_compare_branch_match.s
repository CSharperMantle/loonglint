## Match compare-against-zero branches in both operand orders.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64-unknown-linux -filetype=obj %s -o %t.o
# RUN: ld.lld --entry=_start %t.o -o %t.exe
# RUN: not loonglint %t.exe | FileCheck %s

# CHECK-COUNT-4: [loongarch:control/zero-compare-branch]
# CHECK: 4 finding(s)
# CHECK: 4 loongarch:control/zero-compare-branch

.text
.globl _start
_start:
  beq  $a0, $zero, 8
  beq  $zero, $a1, 8
  bne  $a2, $zero, 8
  bne  $zero, $a3, 8
  nop
  nop
  nop
