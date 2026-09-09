## Match Zbb NOP operations.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=riscv64 -mattr=+m,+a,+f,+d,+c,+zba,+zbb,+zbs -filetype=obj %s -o %t.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.o %t.bin
# RUN: not loonglint --input-format=raw --arch=rv64gc_zba_zbb_zbs %t.bin | FileCheck %s

# CHECK-COUNT-8: [riscv:integer/zbb-nop]
# CHECK: 8 finding(s)
# CHECK: 8 riscv:integer/zbb-nop

.text
.globl _start
_start:
.option push
.option norvc
  min  a0, a0, a0
  minu a1, a1, a1
  max  a2, a2, a2
  maxu a3, a3, a3
  rol  a4, a4, zero
  ror  a5, a5, zero
  rori a6, a6, 0
  andn a7, a7, zero
.option pop
