## Drop redundant load sign extensions.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64-unknown-linux -filetype=obj %s -o %t.o
# RUN: ld.lld --entry=_start %t.o -o %t.exe
# RUN: not loonglint %t.exe | FileCheck %s

## Covers LD.B+EXT.W.B, LD.H+EXT.W.H, and LD.W+ADDI.W rd,rd,0.
# CHECK-COUNT-3: [memory/load-extend]
# CHECK: 3 finding(s)
# CHECK: 3 memory/load-extend

.text
.globl _start
_start:
  ld.b    $t0, $a0, 0
  ext.w.b $t0, $t0
  ld.h    $t1, $a1, 0
  ext.w.h $t1, $t1
  ld.w    $t2, $a2, 0
  addi.w  $t2, $t2, 0
