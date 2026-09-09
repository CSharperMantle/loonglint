## Match slli/srli and srli/slli pairs that fold into an andi mask.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=riscv64 -mattr=+c -filetype=obj %s -o %t.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.o %t.bin
# RUN: not loonglint --input-format=raw --arch=rv64gc %t.bin | FileCheck %s

# CHECK-COUNT-4: [riscv:integer/shift-mask]
# CHECK: 4 finding(s)
# CHECK: 4 riscv:integer/shift-mask

.text
.globl _start
_start:
.option push
.option norvc
  slli a0, a1, 56
  srli a0, a0, 56
  slli a1, a2, 53
  srli a1, a1, 53
  srli a2, a3, 4
  slli a2, a2, 4
  srli a3, a4, 11
  slli a3, a3, 11
.option pop
