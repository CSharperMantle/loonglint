## Match always-true and always-false same-register branches.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=riscv64 -mattr=+c -filetype=obj %s -o %t.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.o %t.bin
# RUN: not loonglint --input-format=raw --arch=rv64gc %t.bin | FileCheck %s

# CHECK-COUNT-7: [riscv:control/degenerate-branch]
# CHECK: 7 finding(s)
# CHECK: 7 riscv:control/degenerate-branch

.text
.globl _start
_start:
  beq  a0, a0, 1f
  addi a1, a1, 1
1:
  bge  a1, a1, 2f
  addi a2, a2, 1
2:
  bgeu a2, a2, 3f
  addi a3, a3, 1
3:
  bne  a3, a3, 4f
  addi a4, a4, 1
4:
  blt  a4, a4, 5f
  addi a5, a5, 1
5:
  bltu a5, a5, 6f
  addi a6, a6, 1
6:
  beqz zero, 7f
  addi a7, a7, 1
7:
