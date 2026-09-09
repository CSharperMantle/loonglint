## Match zext.h formation and redundant zext.h.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=riscv64 -mattr=+m,+a,+f,+d,+c,+zba,+zbb,+zbs -filetype=obj %s -o %t.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.o %t.bin
# RUN: not loonglint --input-format=raw --arch=rv64gc_zba_zbb_zbs %t.bin | FileCheck %s

# CHECK-COUNT-6: [riscv:integer/zbb-zext-h]
# CHECK: 6 finding(s)
# CHECK: 6 riscv:integer/zbb-zext-h

.text
.globl _start
_start:
.option push
.option norvc
  slli   a0, a1, 48
  srli   a0, a0, 48
  lbu    a1, 0(a2)
  zext.h a1, a1
  lhu    a2, 0(a3)
  zext.h a2, a2
  zext.h a3, a4
  zext.h a3, a3
  andi   a4, a5, 100
  zext.h a4, a4
  srli   a5, a6, 50
  zext.h a5, a5
.option pop
