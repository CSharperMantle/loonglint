## Reject standard-defined and custom-reserved HINT encodings and x0 writes
## outside the HINT space.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=riscv32-unknown-elf -mattr=+c -filetype=obj %s -o %t.32.o
# RUN: ld.lld --entry=_start %t.32.o -o %t.32.exe
# RUN: loonglint --filter '^riscv:integer/reserved-hint$' %t.32.exe | FileCheck %s

# RUN: llvm-mc -triple=riscv64-unknown-elf -mattr=+c -filetype=obj %s -o %t.64.o
# RUN: ld.lld --entry=_start %t.64.o -o %t.64.exe
# RUN: loonglint --filter '^riscv:integer/reserved-hint$' %t.64.exe | FileCheck %s

# CHECK: 0 finding(s)

.text
.globl _start
_start:
## Standard-defined HINTs signify intention and stay unreported.
.option push
.option norvc
  slli   zero, zero, 31
  srai   zero, zero, 7
  add    zero, zero, x2
  add    zero, zero, x5
  nop
.option pop
  c.nop
  c.add  zero, x2
## HINTs reserved for custom use stay unreported.
.option push
.option norvc
  slti   zero, a0, 1
  sltiu  zero, a0, 1
  srli   zero, a0, 1
  slli   zero, a0, 1
  slt    zero, a0, a1
  sltu   zero, a0, a1
.option pop
  c.slli a0, 0
  c.srli s0, 0
  c.srai s0, 0
## x0 writes outside the HINT space and unclassified forms stay unreported.
.option push
.option norvc
  lw     zero, 0(a0)
.option pop
  c.li   a0, 0
