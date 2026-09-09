## Reject wrong amounts, sign-extending producers, and narrow masks.
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
  slli   a0, a1, 48
  srli   a0, a0, 47
  lb     a1, 0(a2)
  zext.h a1, a1
  lw     a2, 0(a3)
  zext.h a2, a2
  andi   a3, a4, -1
  zext.h a3, a3
  srli   a4, a5, 40
  zext.h a4, a4
.option pop
