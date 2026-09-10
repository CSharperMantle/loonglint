## Match the Zbkb rotate/andn NOP operations.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=riscv64 -mattr=+m,+a,+c,+zbkb -filetype=obj %s -o %t.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.o %t.bin
# RUN: not loonglint --input-format=raw --arch=rv64gc_zbkb %t.bin | FileCheck %s

# CHECK-COUNT-4: [riscv:integer/nop-zbkb]
# CHECK: 4 finding(s)
# CHECK: 4 riscv:integer/nop-zbkb

.text
.globl _start
_start:
.option push
.option norvc
  rol  a4, a4, zero
  ror  a5, a5, zero
  rori a6, a6, 0
  andn a7, a7, zero
.option pop
