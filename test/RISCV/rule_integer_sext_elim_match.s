## Match redundant sext.b/sext.h after byte- or halfword-extended producers.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=riscv64 -mattr=+m,+a,+f,+d,+c,+zba,+zbb,+zbs -filetype=obj %s -o %t.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.o %t.bin
# RUN: not loonglint --input-format=raw --arch=rv64gc_zba_zbb_zbs %t.bin | FileCheck %s

# CHECK-COUNT-6: [riscv:integer/sext-elim]
# CHECK: 6 finding(s)
# CHECK: 6 riscv:integer/sext-elim

.text
.globl _start
_start:
.option push
.option norvc
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
