## Reject non-1 constants, distinct destinations, and aliasing.
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
  addi t0, zero, 2
  sll  t0, t0, a2
  or   t0, a1, t0
  nop
  addi t1, zero, 1
  sll  t1, t1, a3
  or   a0, a4, t1
  nop
  addi t2, zero, 1
  sll  t2, t2, a5
  or   t2, a0, a1
.option pop
