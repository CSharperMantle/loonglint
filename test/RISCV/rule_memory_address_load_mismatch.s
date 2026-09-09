## Reject out-of-range offsets, distinct destinations, FP loads, and x0 routing.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=riscv64 -mattr=+m,+a,+f,+d,+c -filetype=obj %s -o %t.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.o %t.bin
# RUN: loonglint --input-format=raw --arch=rv64gc %t.bin | FileCheck %s

# CHECK: 0 finding(s)

.text
.globl _start
_start:
.option push
.option norvc
  addi a0, a1, 2047
  lw   a0, 4(a0)
  addi a1, a2, 8
  lw   a2, 4(a1)
  addi a3, a4, 8
  flw  fa0, 4(a3)
  addi x0, a5, 8
  lw   x0, 4(x0)
.option pop
