## Match the future-standard-reserved HINT encodings in RV32 and RV64 code.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=riscv32-unknown-elf -mattr=+c -filetype=obj %s -o %t.32.o
# RUN: ld.lld --entry=_start %t.32.o -o %t.32.exe
# RUN: not loonglint --filter '^riscv:integer/reserved-hint$' %t.32.exe | FileCheck %s

# RUN: llvm-mc -triple=riscv64-unknown-elf -mattr=+c -filetype=obj %s -o %t.64.o
# RUN: ld.lld --entry=_start %t.64.o -o %t.64.exe
# RUN: not loonglint --filter '^riscv:integer/reserved-hint$' %t.64.exe | FileCheck %s

# CHECK-COUNT-21: [riscv:integer/reserved-hint]
# CHECK: 21 finding(s)
# CHECK: 21 riscv:integer/reserved-hint

.text
.globl _start
_start:
.option push
.option norvc
  lui    zero, 1
  auipc  zero, 1
  addi   zero, a0, 1
  addi   zero, zero, 1
  andi   zero, a0, 1
  xori   zero, a0, 1
  add    zero, a0, a1
  add    zero, zero, a6
  sub    zero, a0, a1
  and    zero, a0, a1
  or     zero, a0, a1
  xor    zero, a0, a1
  sll    zero, a0, a1
  srl    zero, a0, a1
  sra    zero, a0, a1
.option pop
## The copy-to-self c.addi row is reserved for future standard use.
  c.addi a0, 0
  c.nop  5
  c.li   zero, 5
  c.lui  zero, 1
  c.mv   zero, a1
  c.add  zero, a1
