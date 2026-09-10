## Match inverted bit-mask materialization + and into bclr.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=riscv64 -mattr=+m,+a,+f,+d,+c,+zba,+zbb,+zbs -filetype=obj %s -o %t.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.o %t.bin
# RUN: not loonglint --input-format=raw --arch=rv64gc_zba_zbb_zbs %t.bin | FileCheck %s

# CHECK: [riscv:integer/bclr]
# CHECK: [riscv:integer/bclr]
# CHECK: [riscv:integer/bclr]
# CHECK: 3 finding(s)
# CHECK: 3 riscv:integer/bclr

.text
.globl _start
_start:
.option push
.option norvc
  addi t0, zero, 1
  sll  t0, t0, a2
  xori t0, t0, -1
  and  t0, a1, t0
  addi t2, zero, 1
  sll  t2, t2, a5
  xori t2, t2, -1
  and  t2, t2, a6
.option pop
  c.li t1, 1
  sll  t1, t1, a3
  xori t1, t1, -1
  and  t1, a4, t1
