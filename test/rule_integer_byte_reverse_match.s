## Fold halfword/byte reversal compositions into REVB.D.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64-unknown-linux -filetype=obj %s -o %t.o
# RUN: ld.lld --entry=_start %t.o -o %t.exe
# RUN: not loonglint %t.exe | FileCheck %s

## Covers both operand orders.
# CHECK-COUNT-2: [integer/byte-reverse]
# CHECK: 2 finding(s)
# CHECK: 2 integer/byte-reverse

.text
.globl _start
_start:
  revb.4h $t0, $a0
  revh.d  $t0, $t0
  revh.d  $t1, $a1
  revb.4h $t1, $t1
