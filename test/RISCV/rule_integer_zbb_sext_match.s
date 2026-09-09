## Match sext.b/sext.h formation, deletion, and idempotence.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=riscv64 -mattr=+m,+a,+f,+d,+c,+zba,+zbb,+zbs -filetype=obj %s -o %t.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.o %t.bin
# RUN: not loonglint --input-format=raw --arch=rv64gc_zba_zbb_zbs %t.bin | FileCheck %s

# CHECK-COUNT-8: [riscv:integer/zbb-sext]
# CHECK: 8 finding(s)
# CHECK: 8 riscv:integer/zbb-sext

.text
.globl _start
_start:
.option push
.option norvc
  slli   a0, a1, 48
  srai   a0, a0, 48
  slli   a1, a2, 56
  srai   a1, a1, 56
  lb     a2, 0(a3)
  sext.b a2, a2
  lh     a3, 0(a4)
  sext.h a3, a3
  lb     a4, 0(a5)
  sext.h a4, a4
  sext.b a5, a6
  sext.b a5, a5
  sext.h a6, a7
  sext.h a6, a6
  sext.b s0, s1
  sext.h s0, s0
.option pop
