## Fold load and zero-extract into unsigned load.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64-unknown-linux -filetype=obj %s -o %t.o
# RUN: ld.lld --entry=_start %t.o -o %t.exe
# RUN: not loonglint %t.exe | FileCheck %s

## Byte, halfword, word zero-extension via BSTRPICK.D.
# CHECK-COUNT-3: [memory/load-zero-extend]
# CHECK: 3 finding(s)
# CHECK: 3 memory/load-zero-extend

.text
.globl _start
_start:
  ld.b       $t0, $a0, 0
  bstrpick.d $t0, $t0, 7, 0
  ld.h       $t1, $a1, 0
  bstrpick.d $t1, $t1, 15, 0
  ld.w       $t2, $a2, 0
  bstrpick.d $t2, $t2, 31, 0
