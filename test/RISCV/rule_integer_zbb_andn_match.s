## Match xori -1 + logic pairs that fold into andn/orn/xnor.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=riscv64 -mattr=+m,+a,+f,+d,+c,+zba,+zbb,+zbs -filetype=obj %s -o %t.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.o %t.bin
# RUN: not loonglint --input-format=raw --arch=rv64gc_zba_zbb_zbs %t.bin | FileCheck %s

# CHECK-COUNT-5: [riscv:integer/zbb-andn]
# CHECK: 5 finding(s)
# CHECK: 5 riscv:integer/zbb-andn

.text
.globl _start
_start:
.option push
.option norvc
  xori t0, a2, -1
  and  t0, a1, t0
  xori t1, a3, -1
  or   t1, a4, t1
  xori t2, a5, -1
  xor  t2, a6, t2
  xori t3, zero, -1
  and  t3, a7, t3
  xori t4, s0, -1
  and  t4, t4, s1
.option pop
