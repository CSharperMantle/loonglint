## Reject distinct destinations and aliasing consumers.
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
  zext.w t0, a3
  add    a0, t0, a4
  nop
  zext.w t1, a4
  add    t1, t1, t1
  nop
  zext.w t2, a5
  add.uw a7, t2, s0
.option pop
