## Reject masks that do not fit the signed 12-bit immediate.
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
  slli a0, a1, 52
  srli a0, a0, 52
  srli a1, a2, 12
  slli a1, a1, 12
  slli a2, a3, 3
  srli a2, a2, 4
  slli a3, a4, 20
  srli a3, a3, 20
.option pop
