## Match sext.b/sext.h formation from the slli/srai pair.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=riscv64 -mattr=+m,+a,+f,+d,+c,+zba,+zbb,+zbs -filetype=obj %s -o %t.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.o %t.bin
# RUN: not loonglint --input-format=raw --arch=rv64gc_zba_zbb_zbs %t.bin | FileCheck %s

# CHECK-COUNT-2: [riscv:integer/sext-form]
# CHECK: 2 finding(s)
# CHECK: 2 riscv:integer/sext-form

.text
.globl _start
_start:
.option push
.option norvc
  slli   a0, a1, 48
  srai   a0, a0, 48
  slli   a1, a2, 56
  srai   a1, a1, 56
.option pop
