## Drop redundant load sign extensions.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64-unknown-linux -filetype=obj %s -o %t.o
# RUN: ld.lld --entry=_start %t.o -o %t.exe
# RUN: not loonglint %t.exe | FileCheck %s

## Covers LD.B+EXT.W.B, LD.H+EXT.W.H, LD.W+{ADDI.W,SLLI.W}, and
## LDPTR.W+{ADDI.W,SLLI.W} rd,rd,0.
# CHECK-COUNT-6: [memory/load-extend]
# CHECK: 6 finding(s)
# CHECK: 6 memory/load-extend

.text
.globl _start
_start:
  ld.b    $t0, $a0, 0
  ext.w.b $t0, $t0
  ld.h    $t1, $a1, 0
  ext.w.h $t1, $t1
  ld.w    $t2, $a2, 0
  addi.w  $t2, $t2, 0
  ld.w    $t3, $a3, 0
  slli.w  $t3, $t3, 0
  ldptr.w $t4, $a4, 8
  addi.w  $t4, $t4, 0
  ldptr.w $t5, $a5, 16
  slli.w  $t5, $t5, 0
