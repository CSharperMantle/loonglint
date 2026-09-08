## Reject non-fusable rotation pairs.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64-unknown-linux -filetype=obj %s -o %t.o
# RUN: ld.lld --entry=_start %t.o -o %t.exe
# RUN: loonglint %t.exe | FileCheck %s

# CHECK: 0 finding(s)

.text
.globl _start
_start:
  rotri.d $t0, $a0, 20
  rotri.w $t0, $t0, 10
  rotri.d $t1, $a0, 20
  rotri.d $t2, $t1, 50
