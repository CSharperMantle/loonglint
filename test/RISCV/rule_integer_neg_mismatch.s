## Reject other immediates and broken chains.
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
  xori a0, a1, -1
  addi a0, a0, 2
  xori a1, a2, -2
  addi a1, a1, 1
  xori a2, a3, -1
  addi a4, a2, 1
.option pop
