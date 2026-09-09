## Match Zba NOP operations with an X0 operand.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=riscv64 -mattr=+m,+a,+f,+d,+c,+zba,+zbb,+zbs -filetype=obj %s -o %t.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.o %t.bin
# RUN: not loonglint --input-format=raw --arch=rv64gc_zba_zbb_zbs %t.bin | FileCheck %s

# CHECK-COUNT-7: [riscv:integer/zba-nop]
# CHECK: 7 finding(s)
# CHECK: 7 riscv:integer/zba-nop

.text
.globl _start
_start:
.option push
.option norvc
  sh1add    a0, zero, a0
  sh2add    a1, zero, a1
  sh3add    a2, zero, a2
  add.uw    a3, zero, a3
  sh1add.uw a4, zero, a4
  sh2add.uw a5, zero, a5
  sh3add.uw a6, zero, a6
.option pop
