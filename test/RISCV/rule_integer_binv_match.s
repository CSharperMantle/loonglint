## Match bit-mask materialization + xor into binv.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=riscv64 -mattr=+m,+a,+f,+d,+c,+zba,+zbb,+zbs -filetype=obj %s -o %t.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.o %t.bin
# RUN: not loonglint --input-format=raw --arch=rv64gc_zba_zbb_zbs %t.bin | FileCheck %s

# CHECK-COUNT-1: [riscv:integer/binv]
# CHECK: 1 finding(s)
# CHECK: 1 riscv:integer/binv

.text
.globl _start
_start:
.option push
.option norvc
  addi t1, zero, 1
  sll  t1, t1, a3
  xor  t1, a4, t1
.option pop
