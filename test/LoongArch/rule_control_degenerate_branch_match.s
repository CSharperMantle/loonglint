## Match degenerate (constant-condition) branches.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64-unknown-linux -filetype=obj %s -o %t.o
# RUN: ld.lld --entry=_start %t.o -o %t.exe
# RUN: not loonglint %t.exe | FileCheck %s

## Always-true forms collapse to `B`; always-false forms are deleted.
# CHECK-COUNT-6: [control/degenerate-branch]
# CHECK: 6 finding(s)
# CHECK: 6 control/degenerate-branch

.text
.globl _start
_start:
  beq  $a0, $a0, 8
  bne  $a1, $a1, 8
  bge  $a2, $a2, 8
  bltu $a3, $a3, 8
  beqz $zero, 8
  bnez $zero, 8
  nop
  nop
  nop
