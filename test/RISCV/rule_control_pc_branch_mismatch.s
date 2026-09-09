## Reject tail forms, mismatched registers, x0 routing, and out-of-range targets.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=riscv64 -mattr=+c -filetype=obj %s -o %t.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.o %t.bin
# RUN: loonglint --input-format=raw --arch=rv64gc %t.bin | FileCheck %s

# CHECK: 0 finding(s)

.text
.globl _start
_start:
.option push
.option norvc
  auipc t0, 0
  jalr  x0, t0, 0
  auipc ra, 0
  jalr  t0, ra, 0
  auipc ra, 0
  jalr  ra, t0, 0
  auipc x0, 0
  jalr  x0, x0, 0
  auipc ra, 0x80000
  jalr  ra, ra, 0
.option pop
