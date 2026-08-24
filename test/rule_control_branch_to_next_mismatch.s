## Reject linking, indirect, and non-next branches.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64-unknown-linux -filetype=obj %s -o %t.o
# RUN: ld.lld --entry=_start %t.o -o %t.exe
# RUN: loonglint %t.exe | FileCheck %s

# CHECK: 0 finding(s)

.text
.globl _start
_start:
  bl 1f
1:
  jirl $zero, $zero, 16
  addi.w $a2, $a2, 1
  b 2f
  addi.w $a3, $a3, 1
2:
  addi.w $a4, $a4, 1
