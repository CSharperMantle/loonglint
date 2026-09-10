## Reject non-NOP Zbkb rotate/andn forms.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=riscv64 -mattr=+m,+a,+c,+zbkb -filetype=obj %s -o %t.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.o %t.bin
# RUN: loonglint --input-format=raw --arch=rv64gc_zbkb %t.bin | FileCheck %s

# CHECK: 0 finding(s)

.text
.globl _start
_start:
.option push
.option norvc
  rol  a1, a1, a2
  nop
  rori a2, a2, 1
  nop
  andn a3, a3, a4
.option pop
