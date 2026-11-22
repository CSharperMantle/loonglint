## Reject atomic consumers whose destination is the materialized register and consumers that never read it (LA64).
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
