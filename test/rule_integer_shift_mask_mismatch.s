## Reject non-redundant shift-amount masks.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64-unknown-linux -filetype=obj %s -o %t.o
# RUN: ld.lld --entry=_start %t.o -o %t.exe
# RUN: loonglint %t.exe | FileCheck %s

# CHECK: 0 finding(s)

.text
.globl _start
_start:
  andi  $t0, $a0, 15
  sll.w $t0, $a1, $t0
  andi  $t1, $a2, 31
  sll.w $t2, $a3, $t1
  andi  $t3, $a4, 31
  sll.w $t3, $t3, $t3
