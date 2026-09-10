## Match the slli/srai pair that forms sext.w.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=riscv64 -mattr=+m,+a,+f,+d,+c -filetype=obj %s -o %t.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.o %t.bin
# RUN: not loonglint --input-format=raw --arch=rv64gc %t.bin | FileCheck %s

# CHECK-COUNT-1: [riscv:integer/sext-w-form]
# CHECK: 1 finding(s)
# CHECK: 1 riscv:integer/sext-w-form

.text
.globl _start
_start:
.option push
.option norvc
  slli a7, s1, 32
  srai a7, a7, 32
.option pop
