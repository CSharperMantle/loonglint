## Match direct branches whose target is the next instruction.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=riscv64 -mattr=+c -filetype=obj %s -o %t.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.o %t.bin
# RUN: not loonglint --input-format=raw --arch=rv64gc %t.bin | FileCheck %s

# CHECK-COUNT-10: [riscv:control/branch-to-next]
# CHECK: 10 finding(s)
# CHECK: 10 riscv:control/branch-to-next

.text
.globl _start
_start:
  beq  a0, a1, 1f
1:
  bne  a1, a2, 2f
2:
  blt  a2, a3, 3f
3:
  bge  a3, a4, 4f
4:
  bltu a4, a5, 5f
5:
  bgeu a5, a6, 6f
6:
.option push
.option norvc
  j    7f
7:
.option pop
  c.j  8f
8:
  c.beqz a0, 9f
9:
  c.bnez s0, 10f
10:
  addi a0, a0, 1
