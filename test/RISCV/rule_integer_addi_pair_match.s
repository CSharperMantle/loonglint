## Match adjacent addi/c.addi pairs that fold into one instruction.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=riscv64 -mattr=+c -filetype=obj %s -o %t.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.o %t.bin
# RUN: not loonglint --input-format=raw --arch=rv64gc %t.bin | FileCheck %s

# CHECK-COUNT-5: [riscv:integer/addi-pair]
# CHECK: 5 finding(s)
# CHECK: 5 riscv:integer/addi-pair

.text
.globl _start
_start:
.option push
.option norvc
  addi a0, a1, 5
  addi a0, a0, 6
  addi a1, a2, 7
  addi a1, a1, -7
  addi a2, a2, 9
  addi a2, a2, -9
.option pop
  addi   a3, a4, 3
  c.addi a3, 4
  c.addi a4, 3
  c.addi a4, 4
