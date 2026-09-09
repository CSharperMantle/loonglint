## Reject sign-extending loads, other masks, and distinct destinations.
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
  lb   a0, 0(a1)
  andi a0, a0, 255
  lbu  a1, 0(a2)
  andi a1, a1, 127
  lbu  a2, 0(a3)
  andi a4, a2, 255
.option pop
