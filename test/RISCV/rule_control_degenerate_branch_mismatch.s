## Reject same-shape branches on distinct registers.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=riscv64 -mattr=+c -filetype=obj %s -o %t.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.o %t.bin
# RUN: loonglint --input-format=raw --arch=rv64gc %t.bin | FileCheck %s

# CHECK: 0 finding(s)

.text
.globl _start
_start:
  beq  a0, a1, 1f
  addi a2, a2, 1
1:
  bne  a1, a2, 2f
  addi a3, a3, 1
2:
  blt  a2, a3, 3f
  addi a4, a4, 1
3:
  bge  a3, a4, 4f
  addi a5, a5, 1
4:
