## Match the RV64-only zero-consuming consumers.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=riscv64 -mattr=+m,+a,+f,+d,+c,+v -filetype=obj %s -o %t.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.o %t.bin
# RUN: not loonglint --input-format=raw --arch=rv64gcv --filter '^riscv:integer/zero-consumer$' %t.bin | FileCheck %s

# CHECK-COUNT-9: [riscv:integer/zero-consumer]
# CHECK: 9 finding(s)
# CHECK: 9 riscv:integer/zero-consumer

.text
.globl _start
_start:
.option push
.option norvc
  addi            a0, zero, 0
  sd              a0, 8(a1)
  subw            a0, a2, a2
  amoswap.d       a1, a0, (a2)
  addw            a0, zero, zero
  amoswap.d.aq    a1, a0, (a2)
  slliw           a0, zero, 1
  amoswap.d.rl    a1, a0, (a2)
  sraiw           a0, zero, 1
  amoswap.d.aqrl  a1, a0, (a2)
  lui             a0, 0
  amoswap.w.aq    a1, a0, (a2)
  sltiu           a0, a1, 0
  amoswap.w.rl    a1, a0, (a2)
  slli            a0, zero, 3
  amoswap.w.aqrl  a1, a0, (a2)
.option pop
  li              a0, 0
  fmv.d.x         fa1, a0
  li              a0, 0
  vsetvli         a1, a0, 8
  li              a0, 0
  vlse8.v         v2, (a0), a3
