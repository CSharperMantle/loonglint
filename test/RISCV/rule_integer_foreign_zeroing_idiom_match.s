## Match the foreign zeroing idioms in RV32 and RV64 code.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=riscv32-unknown-elf -mattr=+c -filetype=obj %s -o %t.32.o
# RUN: ld.lld --entry=_start %t.32.o -o %t.32.exe
# RUN: not loonglint --filter '^riscv:integer/foreign-zeroing-idiom$' %t.32.exe | FileCheck %s

# RUN: llvm-mc -triple=riscv64-unknown-elf -mattr=+c -filetype=obj %s -o %t.64.o
# RUN: ld.lld --entry=_start %t.64.o -o %t.64.exe
# RUN: not loonglint --filter '^riscv:integer/foreign-zeroing-idiom$' %t.64.exe | FileCheck %s

# CHECK-COUNT-24: [riscv:integer/foreign-zeroing-idiom]
# CHECK: 24 finding(s)
# CHECK: 24 riscv:integer/foreign-zeroing-idiom

.text
.globl _start
_start:
.option push
.option norvc
  xor    a0, a0, a0
  xor    a0, a1, a1
  sub    a0, a0, a0
  sub    a2, t0, t0
  and    a0, a0, zero
  and    a0, zero, a0
  andi   a0, a1, 0
  andi   a0, a0, 0
  andi   a0, zero, 5
  slt    a0, a0, a0
  slt    a0, a1, a1
  sltu   a0, a0, a0
  sltu   a0, a1, a1
  sltiu  a0, a1, 0
  slli   a0, zero, 3
  srli   a0, zero, 3
  srai   a0, zero, 3
  sll    a0, zero, a2
  srl    a0, zero, a2
  sra    a0, zero, a2
  lui    a0, 0
  add    a0, zero, zero
  or     a0, zero, zero
.option pop
  c.xor  a0, a0
