## Reject non-ORN NOR-OR shapes.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch32-unknown-linux -filetype=obj %s -o %t.32.o
# RUN: ld.lld --entry=_start %t.32.o -o %t.32.exe
# RUN: loonglint %t.32.exe | FileCheck %s

# RUN: llvm-mc -triple=loongarch64-unknown-linux -filetype=obj %s -o %t.64.o
# RUN: ld.lld --entry=_start %t.64.o -o %t.64.exe
# RUN: loonglint %t.64.exe | FileCheck %s

# CHECK: 0 finding(s)

.text
.globl _start
_start:
  nor  $t0, $a0, $a1
  or   $t0, $a2, $t0
  nor  $t1, $a0, $zero
  or   $t2, $a3, $t1
  nor  $t3, $t3, $zero
  or   $t3, $a4, $t3
  nor  $t4, $a0, $zero
  or   $t4, $a5, $a6
