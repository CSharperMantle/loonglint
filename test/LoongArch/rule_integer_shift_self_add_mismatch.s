## Reject non-fusable double-and-shift pairs.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64-unknown-linux -filetype=obj %s -o %t.o
# RUN: ld.lld --entry=_start %t.o -o %t.exe
# RUN: loonglint %t.exe | FileCheck %s

# CHECK: 0 finding(s)

.text
.globl _start
_start:
  add.d  $t0, $a0, $a1
  slli.d $t0, $t0, 5
  add.d  $t1, $a0, $a0
  slli.d $t2, $t1, 5
  slli.d $t3, $a0, 63
  add.d  $t3, $t3, $t3
