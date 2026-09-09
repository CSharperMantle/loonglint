## Match xori -1 + addi 1 into sub.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=riscv64 -mattr=+c -filetype=obj %s -o %t.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.o %t.bin
# RUN: not loonglint --input-format=raw --arch=rv64gc %t.bin | FileCheck %s

# CHECK-COUNT-2: [riscv:integer/neg]
# CHECK: 2 finding(s)
# CHECK: 2 riscv:integer/neg

.text
.globl _start
_start:
.option push
.option norvc
  xori a0, a1, -1
  addi a0, a0, 1
  xori a1, a1, -1
  addi a1, a1, 1
.option pop
