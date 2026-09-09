## Reject overflowed sums, mixed directions, and non-chaining shifts.
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
  slli a0, a1, 40
  slli a0, a0, 40
  slli a1, a2, 3
  srli a1, a1, 3
  slli a2, a3, 3
  slli a4, a4, 4
.option pop
