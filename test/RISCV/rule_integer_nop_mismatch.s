## Reject non-NOP logical forms and the reserved rd=x0 HINT encodings.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=riscv32-unknown-elf -mattr=+c -filetype=obj %s -o %t.32.o
# RUN: ld.lld --entry=_start %t.32.o -o %t.32.exe
# RUN: loonglint %t.32.exe | FileCheck %s

# RUN: llvm-mc -triple=riscv64-unknown-elf -mattr=+c -filetype=obj %s -o %t.64.o
# RUN: ld.lld --entry=_start %t.64.o -o %t.64.exe
# RUN: loonglint %t.64.exe | FileCheck %s

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
## rd=x0 encodings are HINT space and must stay unreported.
.option push
.option norvc
  add    zero, a0, a1
  add    zero, zero, a1
  slli   zero, zero, 31
  srai   zero, zero, 7
  addi   zero, zero, 1
  ## The canonical 4-byte NOP is never reported.
  nop
.option pop
  ## The canonical 2-byte C.NOP is never reported.
  c.nop
