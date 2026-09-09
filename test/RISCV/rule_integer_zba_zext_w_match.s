## Match zext.w formation, redundant zext.w, and .uw fusions.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=riscv64 -mattr=+m,+a,+f,+d,+c,+zba,+zbb,+zbs -filetype=obj %s -o %t.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.o %t.bin
# RUN: not loonglint --input-format=raw --arch=rv64gc_zba_zbb_zbs %t.bin | FileCheck %s

# CHECK-COUNT-9: [riscv:integer/zba-zext-w]
# CHECK: 9 finding(s)
# CHECK: 9 riscv:integer/zba-zext-w

.text
.globl _start
_start:
.option push
.option norvc
  slli   a0, a1, 32
  srli   a0, a0, 32
  lwu    a1, 0(a2)
  zext.w a1, a1
  lbu    a2, 0(a3)
  zext.w a2, a2
  andi   a3, a4, 100
  zext.w a3, a3
  srli   a4, a5, 40
  zext.w a4, a4
  zext.w t3, s3
  zext.w t3, t3
  zext.w t0, a5
  add    t0, t0, a6
  zext.w t1, a6
  sh1add t1, t1, a7
  zext.w t2, a7
  slli   t2, t2, 3
.option pop
