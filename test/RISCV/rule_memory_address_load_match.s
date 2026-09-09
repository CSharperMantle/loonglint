## Match addi + integer load pairs that fold the offset.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=riscv64 -mattr=+m,+a,+f,+d,+c -filetype=obj %s -o %t.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.o %t.bin
# RUN: not loonglint --input-format=raw --arch=rv64gc %t.bin | FileCheck %s

# CHECK-COUNT-6: [riscv:memory/address-load]
# CHECK: 6 finding(s)
# CHECK: 6 riscv:memory/address-load

.text
.globl _start
_start:
.option push
.option norvc
  addi a0, a1, 8
  lw   a0, 4(a0)
  addi a1, a2, 4
  ld   a1, 8(a1)
  addi a2, a3, -4
  lbu  a2, 8(a2)
  addi a3, a4, 16
  lhu  a3, 4(a3)
  addi a4, a5, 32
  lwu  a4, 4(a4)
  addi a5, a6, 8
  lb   a5, 4(a5)
.option pop
