## Reject mismatched destinations, operations, and broken chains.
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
  ori  a0, a1, 1
  ori  a0, a2, 2
  xori a1, a2, 5
  andi a1, a1, 5
  ori  a2, a3, 1
  ori  a4, a4, 2
.option pop
