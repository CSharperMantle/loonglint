## Reject wrong shift amounts, zero-extending loads, and narrowing.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=riscv64 -mattr=+m,+a,+f,+d,+c,+zba,+zbb,+zbs -filetype=obj %s -o %t.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.o %t.bin
# RUN: loonglint --input-format=raw --arch=rv64gc_zba_zbb_zbs %t.bin | FileCheck %s

# CHECK: 0 finding(s)

.text
.globl _start
_start:
.option push
.option norvc
  slli   a0, a1, 40
  srai   a0, a0, 40
  lbu    a1, 0(a2)
  sext.b a1, a1
  lhu    a2, 0(a3)
  sext.h a2, a2
  lw     a3, 0(a4)
  sext.h a3, a3
  sext.h a4, a5
  sext.b a4, a4
.option pop
