## Reject nonzero materializations, non-reading consumers, and rd=zero producers.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc --triple=riscv32-unknown-elf -mattr=+c,+a -filetype=obj %s -o %t.32.o
# RUN: ld.lld --entry=_start %t.32.o -o %t.32.exe
# RUN: loonglint --filter '^riscv:integer/zero-consumer$' %t.32.exe | FileCheck %s

# CHECK: 0 finding(s)

.attribute arch, "rv32imac"
.text
.globl _start
_start:
.option push
.option norvc
  addi      a0, zero, 5
  sb        a0, 0(a1)
  addi      a0, zero, 0
  amoswap.w a0, a1, (a2)
  addi      a0, zero, 0
  beq       a1, a2, 1f
  addi      zero, zero, 0
  sb        zero, 0(a1)
  xor       zero, a1, a1
  sb        zero, 0(a1)
  addi      a0, zero, 0
  nop
  sb        a0, 0(a1)
  andi      a0, a1, 5
  sb        a0, 0(a1)
1:
.option pop
  li        a0, 0
  c.sw      a0, 0(a1)
  c.xor     a0, a1
  sb        a0, 0(a1)
