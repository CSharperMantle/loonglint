## Reject branches with a distinct target and linking jumps.
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
  jal  ra, 2f
  addi a3, a3, 1
2:
  c.j  3f
  addi a4, a4, 1
3:
