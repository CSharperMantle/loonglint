## Reject non-NOP logical forms and standard-defined HINTs
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=riscv32-unknown-elf -mattr=+c -filetype=obj %s -o %t.32.o
# RUN: ld.lld --entry=_start %t.32.o -o %t.32.exe
# RUN: loonglint --filter '^riscv:integer/nop$' %t.32.exe | FileCheck %s

# RUN: llvm-mc -triple=riscv64-unknown-elf -mattr=+c -filetype=obj %s -o %t.64.o
# RUN: ld.lld --entry=_start %t.64.o -o %t.64.exe
# RUN: loonglint --filter '^riscv:integer/nop$' %t.64.exe | FileCheck %s

# CHECK: 0 finding(s)

.text
.globl _start
_start:
## Base forms: norvc keeps the assembler from compressing them into C forms.
.option push
.option norvc
  addi   a0, a1, 0
  addi   a0, a0, 1
  andi   a0, a0, 1
  andi   a0, a1, -1
  ori    a0, a0, 1
  xori   a0, a0, 1
  slli   a0, a0, 1
  add    a0, a1, zero
  add    a0, zero, a1
  add    a0, a0, a1
  sub    a0, zero, a0
  sub    a0, a1, zero
  sll    a0, a0, a1
  xor    a0, a0, a0
  xor    a0, a1, zero
  or     a0, a0, a1
  and    a0, a1, a2
.option pop
## Compressed forms.
  c.addi a0, 1
  c.mv   a0, a1
  c.andi s0, 1
  c.and  s0, s1
  c.xor  s0, s0
  c.li   a0, 0
## Standard-mandated HINTs must stay unreported.
.option push
.option norvc
  slli   zero, zero, 31
  srai   zero, zero, 7
  ## The canonical 4-byte NOP is never reported.
  nop
.option pop
  ## The canonical 2-byte C.NOP is never reported.
  c.nop
## The compressed copy-to-self HINTs are ruled out by the classifier. The
## c.addi form belongs to integer/reserved-hint; the shifts are unreported.
  c.slli a1, 0
  c.srli s0, 0
  c.srai s0, 0
