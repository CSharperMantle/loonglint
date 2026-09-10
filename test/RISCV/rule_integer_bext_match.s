## Match srli/srl + andi 1 pairs that fold into bexti/bext.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=riscv64 -mattr=+m,+a,+f,+d,+c,+zba,+zbb,+zbs -filetype=obj %s -o %t.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.o %t.bin
# RUN: not loonglint --input-format=raw --arch=rv64gc_zba_zbb_zbs %t.bin | FileCheck %s

# CHECK-COUNT-3: [riscv:integer/bext]
# CHECK: 3 finding(s)
# CHECK: 3 riscv:integer/bext

.text
.globl _start
_start:
.option push
.option norvc
  srli t0, a1, 5
  andi t0, t0, 1
  srli t2, a4, 0
  andi t2, t2, 1
  srl  t1, a2, a3
  andi t1, t1, 1
.option pop
