## Fuse adjacent same-direction shifts.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64-unknown-linux -filetype=obj %s -o %t.o
# RUN: ld.lld --entry=_start %t.o -o %t.exe
# RUN: not loonglint %t.exe | FileCheck %s

## Includes an arithmetic-shift pair whose combined amount clamps to 63.
# CHECK-COUNT-4: [integer/shift-chain]
# CHECK: 4 finding(s)
# CHECK: 4 integer/shift-chain

.text
.globl _start
_start:
  slli.d $t0, $a0, 3
  slli.d $t0, $t0, 4
  srli.w $t1, $a1, 5
  srli.w $t1, $t1, 6
  srai.d $t2, $a2, 40
  srai.d $t2, $t2, 40
  slli.w $t3, $a3, 2
  slli.w $t3, $t3, 3
