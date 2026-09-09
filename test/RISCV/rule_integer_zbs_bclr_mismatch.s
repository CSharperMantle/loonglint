## Reject wrong masks, constants, destinations, and broken chains.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=riscv64 -mattr=+m,+a,+f,+d,+c,+zba,+zbb,+zbs -filetype=obj %s -o %t.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.o %t.bin
# RUN: loonglint --input-format=raw --arch=rv64gc_zba_zbb_zbs %t.bin | FileCheck %s

# CHECK: 0 finding(s)

.text
.globl _start
_start:
.option push
.option norvc
  addi t0, zero, 1
  sll  t0, t0, a2
  xori t0, t0, -2
  and  t0, a1, t0
  addi t1, zero, 2
  sll  t1, t1, a3
  xori t1, t1, -1
  and  t1, a4, t1
  addi t2, zero, 1
  sll  t2, t2, a5
  xori t2, t2, -1
  and  a0, a6, t2
  addi t3, zero, 1
  sll  t3, t3, a7
  xori t3, t3, -1
  and  t3, a0, a1
.option pop
