## Match xori -1 + xor pairs that fold into xnor.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=riscv64 -mattr=+m,+a,+f,+d,+c,+zba,+zbb,+zbs -filetype=obj %s -o %t.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.o %t.bin
# RUN: not loonglint --input-format=raw --arch=rv64gc_zba_zbb_zbs %t.bin | FileCheck %s

# CHECK-COUNT-1: [riscv:integer/not-xor]
# CHECK: 1 finding(s)
# CHECK: 1 riscv:integer/not-xor

.text
.globl _start
_start:
.option push
.option norvc
  xori t2, a5, -1
  xor  t2, a6, t2
.option pop
