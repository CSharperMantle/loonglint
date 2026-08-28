## Reject non-extracting shift-and-mask pairs.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64-unknown-linux -filetype=obj %s -o %t.o
# RUN: ld.lld --entry=_start %t.o -o %t.exe
# RUN: loonglint %t.exe | FileCheck %s

# CHECK: 0 finding(s)

.text
.globl _start
_start:
  slli.d $t0, $a0, 8
  andi   $t0, $t0, 255
  srli.d $t1, $a0, 8
  andi   $t2, $t1, 255
  srli.d $t3, $a0, 8
  andi   $t3, $t3, 128
  srli.d $t4, $a0, 0
  andi   $t4, $t4, 255
  andi   $t5, $a0, 255
  srai.d $t5, $t5, 4
  andi   $t6, $a0, 15
  srli.d $t6, $t6, 8
  andi   $t7, $a0, 5
  srli.d $t7, $t7, 1
