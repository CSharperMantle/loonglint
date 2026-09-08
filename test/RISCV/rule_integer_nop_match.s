## Match width-independent identity operations in RV32 and RV64 code.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=riscv32-unknown-elf -mattr=+c -filetype=obj %s -o %t.32.o
# RUN: ld.lld --entry=_start %t.32.o -o %t.32.exe
# RUN: not loonglint %t.32.exe | FileCheck %s

# RUN: llvm-mc -triple=riscv64-unknown-elf -mattr=+c -filetype=obj %s -o %t.64.o
# RUN: ld.lld --entry=_start %t.64.o -o %t.64.exe
# RUN: not loonglint %t.64.exe | FileCheck %s

# CHECK-COUNT-27: [riscv:integer/nop]
# CHECK: 27 finding(s)
# CHECK: 27 riscv:integer/nop

.text
.globl _start
_start:
## Base forms: norvc keeps the assembler from compressing them into C forms.
.option push
.option norvc
  addi   a0, a0, 0
  andi   a1, a1, -1
  ori    a2, a2, 0
  xori   a3, a3, 0
  slli   a4, a4, 0
  srli   a5, a5, 0
  srai   a6, a6, 0
  add    a7, a7, zero
  add    s1, zero, s1
  sub    s2, s2, zero
  sll    s3, s3, zero
  srl    s4, s4, zero
  sra    s5, s5, zero
  xor    s6, s6, zero
  xor    s7, zero, s7
  or     s8, s8, zero
  or     s9, zero, s9
  or     s10, s10, s10
  and    s11, s11, s11
  nop
.option pop
## Compressed forms.
  c.addi a0, 0
  c.slli a1, 0
  c.srli s0, 0
  c.srai s0, 0
  c.andi s0, -1
  c.mv   a2, a2
  c.and  s1, s1
  c.or   a2, a2
  c.nop
