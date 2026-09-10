## Reject other masks and distinct destinations.
## SPDX-License-Identifier: GPL-3.0-or-later
## The same-register zero shift (`srli t, t, 0`) is a NopRule finding and is
## covered by the NopRule fixtures; it is avoided here to stay rule-isolated.

# RUN: llvm-mc -triple=riscv64 -mattr=+m,+a,+f,+d,+c,+zba,+zbb,+zbs -filetype=obj %s -o %t.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.o %t.bin
# RUN: loonglint --input-format=raw --arch=rv64gc_zba_zbb_zbs %t.bin | FileCheck %s

# CHECK: 0 finding(s)

.text
.globl _start
_start:
.option push
.option norvc
  srli t0, a1, 5
  andi t0, t0, 3
  srl  t2, a3, a4
  andi a0, t2, 1
.option pop
