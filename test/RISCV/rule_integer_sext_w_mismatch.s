## Reject sext.w when the producer does not sign-extend the word.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=riscv64 -mattr=+m,+a,+f,+d,+c -filetype=obj %s -o %t.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.o %t.bin
# RUN: loonglint --input-format=raw --arch=rv64gc %t.bin | FileCheck %s

# CHECK: 0 finding(s)

.text
.globl _start
_start:
.option push
.option norvc
  lwu   a0, 0(a1)
  addiw a0, a0, 0
  auipc a1, 0
  addiw a1, a1, 0
  slli  a2, a3, 32
  srai  a2, a2, 31
  slli  a3, a4, 32
  srai  a5, a5, 32
.option pop
