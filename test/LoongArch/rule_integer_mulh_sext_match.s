## Drop redundant high-multiply sign extensions.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64-unknown-linux -filetype=obj %s -o %t.o
# RUN: ld.lld --entry=_start %t.o -o %t.exe
# RUN: not loonglint %t.exe | FileCheck %s

# CHECK-COUNT-4: [integer/mulh-sext]
# CHECK: 4 finding(s)
# CHECK: 4 integer/mulh-sext

.text
.globl _start
_start:
  mulh.w  $t0, $a0, $a1
  addi.w  $t0, $t0, 0
  mulh.wu $t1, $a2, $a3
  slli.w  $t1, $t1, 0
  mulh.w  $t2, $a4, $a5
  slli.w  $t2, $t2, 0
  mulh.wu $t3, $a6, $a7
  addi.w  $t3, $t3, 0
