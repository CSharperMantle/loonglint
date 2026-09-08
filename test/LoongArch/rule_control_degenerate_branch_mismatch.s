## Reject non-degenerate branches.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64-unknown-linux -filetype=obj %s -o %t.o
# RUN: ld.lld --entry=_start %t.o -o %t.exe
# RUN: loonglint %t.exe | FileCheck %s

# CHECK: 0 finding(s)

.text
.globl _start
_start:
  beq  $a0, $a1, 8
  bne  $a0, $a1, 8
  beqz $a0, 8
  b 8
  nop
  nop
