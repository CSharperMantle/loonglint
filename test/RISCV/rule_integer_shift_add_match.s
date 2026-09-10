## Match slli + add pairs that fold into shNadd.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=riscv64 -mattr=+m,+a,+f,+d,+c,+zba,+zbb,+zbs -filetype=obj %s -o %t.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.o %t.bin
# RUN: not loonglint --input-format=raw --arch=rv64gc_zba_zbb_zbs %t.bin | FileCheck %s

# CHECK-COUNT-4: [riscv:integer/shift-add]
# CHECK: 4 finding(s)
# CHECK: 4 riscv:integer/shift-add

.text
.globl _start
_start:
.option push
.option norvc
  slli a0, a1, 1
  add  a0, a0, a2
  slli a1, a2, 2
  add  a1, a1, a3
  slli a2, a3, 3
  add  a2, a2, a4
  slli a3, a4, 1
  add  a3, a5, a3
.option pop
