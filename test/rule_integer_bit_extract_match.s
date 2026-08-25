## Fold shift-and-mask into BSTRPICK.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64-unknown-linux -filetype=obj %s -o %t.o
# RUN: ld.lld --entry=_start %t.o -o %t.exe
# RUN: not loonglint %t.exe | FileCheck %s

## Covers SRLI/SRAI in .W and .D with contiguous low masks.
# CHECK-COUNT-3: [integer/bit-extract]
# CHECK: 3 finding(s)
# CHECK: 3 integer/bit-extract

.text
.globl _start
_start:
  srli.d $t0, $a0, 8
  andi   $t0, $t0, 255
  srli.w $t1, $a1, 4
  andi   $t1, $t1, 15
  srai.d $t2, $a2, 12
  andi   $t2, $t2, 4095
