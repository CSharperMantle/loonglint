## Reject wrong amounts, sign-extending producers, and broken chains.
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
  slli   a0, a1, 32
  srli   a0, a0, 31
  lb     a1, 0(a2)
  zext.w a1, a1
  lw     a2, 0(a3)
  zext.w a2, a2
  zext.w t0, a3
  add    a0, t0, a4
  zext.w t1, a4
  add    t1, t1, t1
  andi   a5, a6, -1
  zext.w a5, a5
  add.uw a6, s1, s2
  zext.w a6, a6
.option pop
