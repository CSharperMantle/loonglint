## Match NOR-OR pairs folding to ORN.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch32-unknown-linux -filetype=obj %s -o %t.32.o
# RUN: ld.lld --entry=_start %t.32.o -o %t.32.exe
# RUN: not loonglint %t.32.exe | FileCheck %s

# RUN: llvm-mc -triple=loongarch64-unknown-linux -filetype=obj %s -o %t.64.o
# RUN: ld.lld --entry=_start %t.64.o -o %t.64.exe
# RUN: not loonglint %t.64.exe | FileCheck %s

# CHECK-COUNT-4: [integer/or-not]
# CHECK: 4 finding(s)
# CHECK: 4 integer/or-not

.text
.globl _start
_start:
  nor  $t0, $a0, $zero
  or   $t0, $a1, $t0
  nor  $t1, $zero, $a2
  or   $t1, $t1, $a3
  nor  $t2, $a4, $zero
  or   $t2, $t2, $a5
  nor  $t3, $zero, $a6
  or   $t3, $a7, $t3
