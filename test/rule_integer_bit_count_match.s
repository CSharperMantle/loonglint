## Match complemented bit-count pairs folding to CLO/CTO.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64-unknown-linux -filetype=obj %s -o %t.o
# RUN: ld.lld --entry=_start %t.o -o %t.exe
# RUN: not loonglint %t.exe | FileCheck %s

## Covers CLZ.W/CTZ.W/CLZ.D/CTZ.D and both NOR $zero operand orders.
# CHECK-COUNT-4: [integer/bit-count]
# CHECK: 4 finding(s)
# CHECK: 4 integer/bit-count

.text
.globl _start
_start:
  nor   $t0, $a0, $zero
  clz.w $t0, $t0
  nor   $t1, $zero, $a1
  ctz.w $t1, $t1
  nor   $t2, $a2, $zero
  clz.d $t2, $t2
  nor   $t3, $zero, $a3
  ctz.d $t3, $t3
