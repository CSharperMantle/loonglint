## Reject nonzero compares and already-folded zero branches.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64-unknown-linux -filetype=obj %s -o %t.o
# RUN: ld.lld --entry=_start %t.o -o %t.exe
# RUN: loonglint %t.exe | FileCheck %s

# CHECK: 0 finding(s)

.text
.globl _start
_start:
  beq  $a0, $a1, 8
  bne  $a2, $a3, 8
  beqz $a4, 8
  bnez $a5, 8
  bge  $a6, $zero, 8
  nop
  nop
  nop
