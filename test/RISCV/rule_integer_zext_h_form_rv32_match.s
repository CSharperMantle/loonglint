## Match RV32 zext.h formation from the 16-bit shift pair.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=riscv32 -mattr=+m,+a,+c,+zbb,+zbs -filetype=obj %s -o %t.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.o %t.bin
# RUN: not loonglint --input-format=raw --arch=rv32imac_zbb_zbs %t.bin | FileCheck %s

# CHECK-COUNT-1: [riscv:integer/zext-h-form]
# CHECK: 1 finding(s)
# CHECK: 1 riscv:integer/zext-h-form

.text
.globl _start
_start:
.option push
.option norvc
  slli a0, a1, 16
  srli a0, a0, 16
.option pop
