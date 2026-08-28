## Fold shift-and-mask into BSTRPICK.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64-unknown-linux -filetype=obj %s -o %t.o
# RUN: ld.lld --entry=_start %t.o -o %t.exe
# RUN: not loonglint %t.exe | FileCheck %s

## Covers SRLI/SRAI in .W and .D with contiguous low masks (shift-first),
## plus the mask-first order (ANDI then SRLI).
# CHECK-COUNT-5: [integer/bit-extract]
# CHECK: 5 finding(s)
# CHECK: 5 integer/bit-extract

.text
.globl _start
_start:
  srli.d $t0, $a0, 8
  andi   $t0, $t0, 255
  srli.w $t1, $a1, 4
  andi   $t1, $t1, 15
  srai.d $t2, $a2, 12
  andi   $t2, $t2, 4095
  andi   $t3, $a3, 4095
  srli.d $t3, $t3, 4
  andi   $t4, $a4, 255
  srli.w $t4, $t4, 2
