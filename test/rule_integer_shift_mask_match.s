## Drop redundant shift-amount masks.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64-unknown-linux -filetype=obj %s -o %t.o
# RUN: ld.lld --entry=_start %t.o -o %t.exe
# RUN: not loonglint %t.exe | FileCheck %s

## Covers SLL/SRL/SRA in .W (mask 31) and .D (mask 63).
# CHECK-COUNT-4: [integer/shift-mask]
# CHECK: 4 finding(s)
# CHECK: 4 integer/shift-mask

.text
.globl _start
_start:
  andi  $t0, $a0, 31
  sll.w $t0, $a1, $t0
  andi  $t1, $a2, 63
  srl.d $t1, $a3, $t1
  andi  $t2, $a4, 31
  sra.w $t2, $a5, $t2
  andi  $t3, $a6, 63
  sll.d $t3, $a7, $t3
