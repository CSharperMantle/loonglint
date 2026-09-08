## Match LA64 doubleword-width identity operations.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64-unknown-linux -filetype=obj %s -o %t.o
# RUN: ld.lld --entry=_start %t.o -o %t.exe
# RUN: not loonglint %t.exe | FileCheck %s

# CHECK-COUNT-7: [integer/nop-la64]
# CHECK: 7 finding(s)
# CHECK: 7 integer/nop-la64

.text
.globl _start
_start:
  add.d   $a0, $a0, $zero
  sub.d   $a1, $a1, $zero
  addi.d  $a2, $a2, 0
  slli.d  $a3, $a3, 0
  srli.d  $a4, $a4, 0
  srai.d  $a5, $a5, 0
  rotri.d $a6, $a6, 0
