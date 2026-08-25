## Reject non-NOR OR-NOT shapes.
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
  or   $t0, $a0, $a1
  nor  $t1, $t1, $zero
  or   $t2, $a2, $a3
  nor  $t2, $t2, $a4
