## Reject non-fusable ADDI pairs.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64-unknown-linux -filetype=obj %s -o %t.o
# RUN: ld.lld --entry=_start %t.o -o %t.exe
# RUN: loonglint %t.exe | FileCheck %s

# CHECK: 0 finding(s)

.text
.globl _start
_start:
  addi.w $t0, $a0, 5
  addi.w $t1, $t0, 3
  addi.w $t2, $a0, 5
  addi.w $t2, $t2, 0
  addi.w $t3, $a0, 2000
  addi.w $t3, $t3, 100
