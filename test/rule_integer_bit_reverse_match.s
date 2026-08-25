## Fold byte/bit reversal compositions into BITREV.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64-unknown-linux -filetype=obj %s -o %t.o
# RUN: ld.lld --entry=_start %t.o -o %t.exe
# RUN: not loonglint %t.exe | FileCheck %s

## Covers both operand orders and 4B/8B widths.
# CHECK-COUNT-4: [integer/bit-reverse]
# CHECK: 4 finding(s)
# CHECK: 4 integer/bit-reverse

.text
.globl _start
_start:
  revb.2w  $t0, $a0
  bitrev.w $t0, $t0
  bitrev.w $t1, $a1
  revb.2w  $t1, $t1
  revb.d   $t2, $a2
  bitrev.d $t2, $t2
  bitrev.d $t3, $a3
  revb.d   $t3, $t3
