## Match consumers reading a just-materialized zero.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc --triple=riscv32-unknown-elf -mattr=+c,+a -filetype=obj %s -o %t.32.o
# RUN: ld.lld --entry=_start %t.32.o -o %t.32.exe
# RUN: not loonglint --filter '^riscv:integer/zero-consumer$' %t.32.exe | FileCheck %s

# CHECK-COUNT-12: [riscv:integer/zero-consumer]
# CHECK: 12 finding(s)
# CHECK: 12 riscv:integer/zero-consumer

.attribute arch, "rv32imac"
.text
.globl _start
_start:
.option push
.option norvc
  addi            a0, zero, 0
  sb              a0, 0(a1)
  addi            a1, zero, 0
  sw              a2, 8(a1)
  addi            a2, zero, 0
  sh              a2, 0(a2)
  mv              a3, zero
  bne             a3, a4, 1f
  xor             a0, a5, a5
  bltu            a6, a0, 1f
  andi            a0, a0, 0
  beq             a0, a1, 1f
  lui             a0, 0
  amoswap.w.aq    a1, a0, (a2)
  sltiu           a0, a1, 0
  amoswap.w.rl    a1, a0, (a2)
  slli            a0, zero, 3
  amoswap.w.aqrl  a1, a0, (a2)
1:
.option pop
  li              a0, 0
  sw              a0, 0(t2)
  sltu            a0, a1, a1
  amoswap.w       a1, a0, (a0)
  mv              a0, zero
  sub             a1, a0, a2
