## Match redundant sext.w after sign-extending producers and the slli/srai idiom.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=riscv64 -mattr=+m,+a,+f,+d,+c -filetype=obj %s -o %t.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.o %t.bin
# RUN: not loonglint --input-format=raw --arch=rv64gc %t.bin | FileCheck %s

# CHECK-COUNT-7: [riscv:integer/sext-w]
# CHECK: 7 finding(s)
# CHECK: 7 riscv:integer/sext-w

.text
.globl _start
_start:
.option push
.option norvc
  addw     a0, a1, a2
  addiw    a0, a0, 0
  lw       a1, 0(a2)
  addiw    a1, a1, 0
  lui      a3, 1
  addiw    a3, a3, 0
  slt      a4, a5, a6
  addiw    a4, a4, 0
  mulw     a5, a6, a7
  addiw    a5, a5, 0
  amoadd.w a6, a7, (s0)
  addiw    a6, a6, 0
  slli     a7, s1, 32
  srai     a7, a7, 32
.option pop
