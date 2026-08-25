## Fold left/right shift pairs into zero-extending BSTRPICK.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64-unknown-linux -filetype=obj %s -o %t.o
# RUN: ld.lld --entry=_start %t.o -o %t.exe
# RUN: not loonglint %t.exe | FileCheck %s

## Byte, halfword, and word zero-extension.
# CHECK-COUNT-3: [integer/zero-extend]
# CHECK: 3 finding(s)
# CHECK: 3 integer/zero-extend

.text
.globl _start
_start:
  slli.d $t0, $a0, 56
  srli.d $t0, $t0, 56
  slli.d $t1, $a1, 48
  srli.d $t1, $t1, 48
  slli.w $t2, $a2, 24
  srli.w $t2, $t2, 24
