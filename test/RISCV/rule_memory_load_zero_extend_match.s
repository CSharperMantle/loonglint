## Match redundant andi 255 after a zero-extending byte load.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=riscv64 -mattr=+c -filetype=obj %s -o %t.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.o %t.bin
# RUN: not loonglint --input-format=raw --arch=rv64gc %t.bin | FileCheck %s

# CHECK-COUNT-2: [riscv:memory/load-extend]
# CHECK: 2 finding(s)
# CHECK: 2 riscv:memory/load-extend

.text
.globl _start
_start:
.option push
.option norvc
  lbu  a0, 0(a1)
  andi a0, a0, 255
  lbu  a1, 4(a2)
  andi a1, a1, 255
.option pop
