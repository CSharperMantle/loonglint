## Reject non-foldable load zero-extends.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64-unknown-linux -filetype=obj %s -o %t.o
# RUN: ld.lld --entry=_start %t.o -o %t.exe
# RUN: loonglint %t.exe | FileCheck %s

## Includes the LA64 .W pick, which is not a zero-extend here.
# CHECK: 0 finding(s)

.text
.globl _start
_start:
  ld.b       $t0, $a0, 0
  bstrpick.d $t1, $t0, 7, 0
  ld.h       $t2, $a1, 0
  bstrpick.d $t2, $t2, 15, 4
  ld.b       $t3, $a0, 0
  bstrpick.w $t3, $t3, 7, 0
