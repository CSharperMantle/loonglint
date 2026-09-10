## Match Zbb min/max NOP operations.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=riscv64 -mattr=+m,+a,+f,+d,+c,+zbb -filetype=obj %s -o %t.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.o %t.bin
# RUN: not loonglint --input-format=raw --arch=rv64gc_zbb %t.bin | FileCheck %s

# CHECK-COUNT-4: [riscv:integer/nop-zbb]
# CHECK: 4 finding(s)
# CHECK: 4 riscv:integer/nop-zbb

.text
.globl _start
_start:
.option push
.option norvc
  min  a0, a0, a0
  minu a1, a1, a1
  max  a2, a2, a2
  maxu a3, a3, a3
.option pop
