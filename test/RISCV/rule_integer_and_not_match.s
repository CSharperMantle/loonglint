## Match xori -1 + and pairs that fold into andn.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=riscv64 -mattr=+m,+a,+f,+d,+c,+zba,+zbb,+zbs -filetype=obj %s -o %t.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.o %t.bin
# RUN: not loonglint --input-format=raw --arch=rv64gc_zba_zbb_zbs %t.bin | FileCheck %s

# CHECK-COUNT-3: [riscv:integer/and-not]
# CHECK: 3 finding(s)
# CHECK: 3 riscv:integer/and-not

.text
.globl _start
_start:
.option push
.option norvc
  xori t0, a2, -1
  and  t0, a1, t0
  xori t3, zero, -1
  and  t3, a7, t3
  xori t4, s0, -1
  and  t4, t4, s1
.option pop
