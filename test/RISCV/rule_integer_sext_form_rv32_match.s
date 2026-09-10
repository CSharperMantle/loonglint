## Match RV32 sext.b/sext.h formation from the 16/24-bit shift amounts.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=riscv32 -mattr=+m,+a,+c,+zbb,+zbs -filetype=obj %s -o %t.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.o %t.bin
# RUN: not loonglint --input-format=raw --arch=rv32imac_zbb_zbs %t.bin | FileCheck %s

# CHECK-COUNT-2: [riscv:integer/sext-form]
# CHECK: 2 finding(s)
# CHECK: 2 riscv:integer/sext-form

.text
.globl _start
_start:
.option push
.option norvc
  slli a0, a1, 16
  srai a0, a0, 16
  slli a1, a2, 24
  srai a1, a1, 24
.option pop
