## Fold ADD and zero-offset load into indexed load.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64-unknown-linux -filetype=obj %s -o %t.o
# RUN: ld.lld --entry=_start %t.o -o %t.exe
# RUN: not loonglint %t.exe | FileCheck %s

## Covers B/H/W/D/BU/HU/WU widths.
# CHECK-COUNT-7: [memory/indexed-load]
# CHECK: 7 finding(s)
# CHECK: 7 memory/indexed-load

.text
.globl _start
_start:
  add.d $t0, $a0, $a1
  ld.b  $t0, $t0, 0
  add.d $t1, $a0, $a1
  ld.h  $t1, $t1, 0
  add.d $t2, $a0, $a1
  ld.w  $t2, $t2, 0
  add.d $t3, $a0, $a1
  ld.d  $t3, $t3, 0
  add.d $t4, $a0, $a1
  ld.bu $t4, $t4, 0
  add.d $t5, $a0, $a1
  ld.hu $t5, $t5, 0
  add.d $t6, $a0, $a1
  ld.wu $t6, $t6, 0
