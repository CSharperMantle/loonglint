## Match bit-mask materialization + or/xor into bset/binv.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=riscv64 -mattr=+m,+a,+f,+d,+c,+zba,+zbb,+zbs -filetype=obj %s -o %t.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.o %t.bin
# RUN: not loonglint --input-format=raw --arch=rv64gc_zba_zbb_zbs %t.bin | FileCheck %s

# CHECK-COUNT-4: [riscv:integer/zbs-bset]
# CHECK: 4 finding(s)
# CHECK: 4 riscv:integer/zbs-bset

.text
.globl _start
_start:
.option push
.option norvc
  addi t0, zero, 1
  sll  t0, t0, a2
  or   t0, a1, t0
  addi t1, zero, 1
  sll  t1, t1, a3
  xor  t1, a4, t1
  addi t3, zero, 1
  sll  t3, t3, s0
  or   t3, t3, s1
.option pop
  c.li t2, 1
  sll  t2, t2, a5
  or   t2, a6, t2
