## Match same-direction shift pairs, including compressed and mixed forms.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=riscv64 -mattr=+c -filetype=obj %s -o %t.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.o %t.bin
# RUN: not loonglint --input-format=raw --arch=rv64gc %t.bin | FileCheck %s

# CHECK-COUNT-8: [riscv:integer/shift-chain]
# CHECK: 8 finding(s)
# CHECK: 8 riscv:integer/shift-chain

.text
.globl _start
_start:
.option push
.option norvc
  slli a0, a1, 3
  slli a0, a0, 4
  srli a1, a2, 5
  srli a1, a1, 6
  srai a2, a3, 7
  srai a2, a2, 8
  srai a3, a4, 40
  srai a3, a3, 40
.option pop
  c.slli a4, 3
  c.slli a4, 4
  c.srli a5, 5
  c.srli a5, 6
  c.srai s0, 40
  c.srai s0, 40
  slli a5, a6, 3
  c.slli a5, 4
