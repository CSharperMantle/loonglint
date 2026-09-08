## Fuse adjacent rotations.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64-unknown-linux -filetype=obj %s -o %t.o
# RUN: ld.lld --entry=_start %t.o -o %t.exe
# RUN: not loonglint %t.exe | FileCheck %s

# CHECK-COUNT-3: [integer/rotate-combine]
# CHECK: 3 finding(s)
# CHECK: 3 integer/rotate-combine

.text
.globl _start
_start:
  rotri.d $t0, $a0, 20
  rotri.d $t0, $t0, 50
  rotri.w $t1, $a1, 10
  rotri.w $t1, $t1, 25
  rotri.d $t2, $a2, 63
  rotri.d $t2, $t2, 63
