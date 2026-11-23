## Reject consumers with no rewritable source operand.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64-unknown-linux -filetype=obj %s -o %t.o
# RUN: ld.lld --entry=_start %t.o -o %t.exe
# RUN: loonglint --filter '^loongarch:integer/zero-consumer$' %t.exe | FileCheck %s

# CHECK: 0 finding(s)

.text
.globl _start
_start:
  or $a0, $zero, $zero
  amcas.w $a0, $a1, $a2
  or $a0, $zero, $zero
  .word 0x38611144
  or $t8, $zero, $zero
  add.w $t8, $a1, $a2
  ori $a0, $zero, 0
  lu32i.d $a0, 8
  li.d $t1, 42
  li.d $t0, 0
1:
  bgeu $t0, $t1, 2f
  addi.d $t0, $t0, 1
  b 1b
2:
