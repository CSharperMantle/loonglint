## Reject non-foldable indexed-load pairs.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64-unknown-linux -filetype=obj %s -o %t.o
# RUN: ld.lld --entry=_start %t.o -o %t.exe
# RUN: loonglint %t.exe | FileCheck %s

# CHECK: 0 finding(s)

.text
.globl _start
_start:
  add.d $t0, $a0, $a1
  ld.b  $t1, $t0, 0
  add.d $t0, $a0, $t0
  ld.b  $t0, $t0, 0
  add.d $t2, $a0, $a1
  ld.b  $t2, $t2, 8
