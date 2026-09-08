## Reject non-CLO/CTO complemented bit-count shapes.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64-unknown-linux -filetype=obj %s -o %t.o
# RUN: ld.lld --entry=_start %t.o -o %t.exe
# RUN: loonglint %t.exe | FileCheck %s

# CHECK: 0 finding(s)

.text
.globl _start
_start:
  nor   $t0, $a0, $a1
  clz.w $t0, $t0
  nor   $t1, $a0, $zero
  clz.w $t2, $t1
  nor   $t3, $a0, $zero
  ctz.w $t3, $a3
