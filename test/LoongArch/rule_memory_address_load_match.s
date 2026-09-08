## Match address ADDI with all LA64 integer loads that use immediate offsets.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64-unknown-linux -filetype=obj %s -o %t.o
# RUN: ld.lld --entry=_start %t.o -o %t.exe
# RUN: not loonglint %t.exe | FileCheck %s

# CHECK: ld.b $t0, $a0, 24
# CHECK: ld.h $t1, $a1, 24
# CHECK: ld.w $t2, $a2, 24
# CHECK: ld.d $t3, $a3, 24
# CHECK: ld.bu $t4, $a4, 24
# CHECK: ld.hu $t5, $a5, 24
# CHECK: ld.wu $t6, $a6, 24
# CHECK: ldptr.w $t7, $a7, 24
# CHECK: ldptr.d $s0, $a0, 24
# CHECK: 9 finding(s)
# CHECK: 9 memory/address-load

.text
.globl _start
_start:
  addi.d $t0, $a0, 8
  ld.b   $t0, $t0, 16
  addi.d $t1, $a1, 8
  ld.h   $t1, $t1, 16
  addi.d $t2, $a2, 8
  ld.w   $t2, $t2, 16
  addi.d $t3, $a3, 8
  ld.d   $t3, $t3, 16
  addi.d $t4, $a4, 8
  ld.bu  $t4, $t4, 16
  addi.d $t5, $a5, 8
  ld.hu  $t5, $t5, 16
  addi.d $t6, $a6, 8
  ld.wu  $t6, $t6, 16
  addi.d $t7, $a7, 8
  ldptr.w $t7, $t7, 16
  addi.d $s0, $a0, 8
  ldptr.d $s0, $s0, 16
