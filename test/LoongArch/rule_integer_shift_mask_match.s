## Drop redundant shift-amount masks.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64-unknown-linux -filetype=obj %s -o %t.o
# RUN: ld.lld --entry=_start %t.o -o %t.exe
# RUN: not loonglint %t.exe | FileCheck %s

## Covers SLL/SRL/SRA/ROTR in .W and .D, superset count masks, and the
## LA64 BSTRPICK.D count-mask form.
# CHECK-COUNT-10: [integer/shift-mask]
# CHECK: 10 finding(s)
# CHECK: 10 integer/shift-mask

.text
.globl _start
_start:
  andi       $t0, $a0, 31
  sll.w      $t0, $a1, $t0
  andi       $t1, $a2, 63
  srl.d      $t1, $a3, $t1
  andi       $t2, $a4, 31
  sra.w      $t2, $a5, $t2
  andi       $t3, $a6, 63
  sll.d      $t3, $a7, $t3
  andi       $t4, $t1, 2047
  srl.w      $t4, $t0, $t4
  andi       $t5, $t2, 4095
  sra.d      $t5, $t3, $t5
  andi       $t6, $t3, 31
  rotr.w     $t6, $t2, $t6
  andi       $t7, $t4, 63
  rotr.d     $t7, $t5, $t7
  bstrpick.d $s0, $t5, 5, 0
  sll.d      $s0, $t1, $s0
  bstrpick.d $s1, $t7, 4, 0
  sll.w      $s1, $t6, $s1
