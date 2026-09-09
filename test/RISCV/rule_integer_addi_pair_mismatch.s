## Reject addi pairs that overflow the immediate or do not chain.
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
  addi a0, a1, 2047
  addi a0, a0, 1
  addi a1, a2, 5
  addi a2, a2, 6
  addi a3, a4, 5
  addi a3, a5, 6
.option pop
  c.addi a4, 3
  addi   a4, a5, 4
