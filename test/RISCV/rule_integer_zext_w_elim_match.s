## Match redundant zext.w after width-bounded producers.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=riscv64 -mattr=+m,+a,+f,+d,+c,+zba,+zbb,+zbs -filetype=obj %s -o %t.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.o %t.bin
# RUN: not loonglint --input-format=raw --arch=rv64gc_zba_zbb_zbs %t.bin | FileCheck %s

# CHECK-COUNT-5: [riscv:integer/zext-w-elim]
# CHECK: 5 finding(s)
# CHECK: 5 riscv:integer/zext-w-elim

.text
.globl _start
_start:
.option push
.option norvc
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
.option pop
