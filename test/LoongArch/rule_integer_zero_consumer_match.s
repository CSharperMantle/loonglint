## Match consumers of a materialized zero with the '$zero'-register replacement (LA64).
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64-unknown-linux -filetype=obj %s -o %t.o
# RUN: ld.lld --entry=_start %t.o -o %t.exe
# RUN: not loonglint --filter '^loongarch:integer/zero-consumer$' %t.exe | FileCheck %s

# CHECK: 10 finding(s)
# CHECK: 10 loongarch:integer/zero-consumer

.text
.globl _start
_start:
  or $t0, $zero, $zero
  st.w $t0, $a0, 0
  ori $t0, $zero, 0
  beq $t0, $a1, 12
  addi.w $t1, $zero, 0
  add.w $a2, $t1, $a2
  sub.w $t2, $t3, $t3
  ld.w $a3, $t2, 8
  lu12i.w $t4, 0
  jirl $a4, $t4, 0
  mul.w $t5, $zero, $t6
  st.b $t5, $a7, 3
  addi.d $t7, $zero, 0
  add.d $a2, $t7, $t7
  or $t1, $zero, $zero
  add.w $t1, $t1, $a2
  lu12i.w $t4, 0
  sc.w $a5, $t4, 0
  ori $t8, $zero, 0
  add.d $t8, $t8, $a1
