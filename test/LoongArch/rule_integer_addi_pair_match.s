## Fuse adjacent ADDI pairs.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64-unknown-linux -filetype=obj %s -o %t.o
# RUN: ld.lld --entry=_start %t.o -o %t.exe
# RUN: not loonglint %t.exe | FileCheck %s

## Covers .W and .D with positive and negative immediates.
# CHECK-COUNT-3: [integer/addi-pair]
# CHECK: 3 finding(s)
# CHECK: 3 integer/addi-pair

.text
.globl _start
_start:
  addi.w $t0, $a0, 5
  addi.w $t0, $t0, 3
  addi.d $t1, $a1, -5
  addi.d $t1, $t1, 3
  addi.w $t2, $a2, 100
  addi.w $t2, $t2, 200
