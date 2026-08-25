## Match LA32 word-width identity operations.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch32-unknown-linux -filetype=obj %s -o %t.o
# RUN: ld.lld --entry=_start %t.o -o %t.exe
# RUN: not loonglint %t.exe | FileCheck %s

# CHECK-COUNT-7: [integer/nop-la32]
# CHECK: 7 finding(s)
# CHECK: 7 integer/nop-la32

.text
.globl _start
_start:
  add.w   $a0, $a0, $zero
  sub.w   $a1, $a1, $zero
  addi.w  $a2, $a2, 0
  slli.w  $a3, $a3, 0
  srli.w  $a4, $a4, 0
  srai.w  $a5, $a5, 0
  rotri.w $a6, $a6, 0
