## Reject non-redundant high-multiply extensions.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64-unknown-linux -filetype=obj %s -o %t.o
# RUN: ld.lld --entry=_start %t.o -o %t.exe
# RUN: loonglint %t.exe | FileCheck %s

# CHECK: 0 finding(s)

.text
.globl _start
_start:
  mulh.w $t0, $a0, $a1
  addi.w $t1, $t0, 0
  mulh.w $t2, $a0, $a1
  addi.w $t2, $t2, 1
