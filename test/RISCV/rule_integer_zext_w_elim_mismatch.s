## Reject sign-extending producers, wide masks, and non-X0 addends.
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
  lb     a1, 0(a2)
  zext.w a1, a1
  nop
  lw     a2, 0(a3)
  zext.w a2, a2
  nop
  andi   a5, a6, -1
  zext.w a5, a5
  nop
  add.uw a6, s1, s2
  zext.w a6, a6
.option pop
