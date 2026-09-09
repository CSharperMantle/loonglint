## Reject out-of-range shifts, aliasing, and broken chains.
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
  slli a0, a1, 4
  add  a0, a0, a2
  slli a1, a2, 1
  add  a1, a1, a1
  slli a2, a3, 1
  add  a2, a4, a5
.option pop
