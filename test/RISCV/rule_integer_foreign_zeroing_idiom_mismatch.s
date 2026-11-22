## Reject non-zeroing forms and rd=zero HINT encodings.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=riscv32-unknown-elf -mattr=+c -filetype=obj %s -o %t.32.o
# RUN: ld.lld --entry=_start %t.32.o -o %t.32.exe
# RUN: loonglint --filter '^riscv:integer/foreign-zeroing-idiom$' %t.32.exe | FileCheck %s

# RUN: llvm-mc -triple=riscv64-unknown-elf -mattr=+c -filetype=obj %s -o %t.64.o
# RUN: ld.lld --entry=_start %t.64.o -o %t.64.exe
# RUN: loonglint --filter '^riscv:integer/foreign-zeroing-idiom$' %t.64.exe | FileCheck %s

# CHECK: 0 finding(s)

.text
.globl _start
_start:
.option push
.option norvc
  xor    a0, a1, a2
  sub    a0, a1, a2
  and    a0, a1, a2
  andi   a0, a1, 5
  slt    a0, a1, a2
  sltu   a0, a1, a2
  sltiu  a0, a1, 1
  slli   a0, a1, 3
  lui    a0, 1
  add    a0, zero, a1
  slt    zero, a1, a1
  sltu   zero, a1, a1
  sltiu  zero, a1, 0
  xor    zero, a1, a1
  sub    zero, a1, a1
  and    zero, a1, zero
  andi   zero, a1, 0
  slli   zero, a1, 3
  lui    zero, 1
  add    zero, zero, zero
  or     zero, zero, a1
.option pop
  c.xor  a0, a1
