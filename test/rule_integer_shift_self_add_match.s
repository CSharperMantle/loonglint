## Fuse double-and-shift pairs.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64-unknown-linux -filetype=obj %s -o %t.o
# RUN: ld.lld --entry=_start %t.o -o %t.exe
# RUN: not loonglint %t.exe | FileCheck %s

## Covers both operand orders and .W/.D widths.
# CHECK-COUNT-4: [integer/shift-self-add]
# CHECK: 4 finding(s)
# CHECK: 4 integer/shift-self-add

.text
.globl _start
_start:
  add.d  $t0, $a0, $a0
  slli.d $t0, $t0, 5
  slli.d $t1, $a1, 3
  add.d  $t1, $t1, $t1
  add.w  $t2, $a2, $a2
  slli.w $t2, $t2, 4
  slli.w $t3, $a3, 2
  add.w  $t3, $t3, $t3
