## Reject non-redundant load extensions.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64-unknown-linux -filetype=obj %s -o %t.o
# RUN: ld.lld --entry=_start %t.o -o %t.exe
# RUN: loonglint %t.exe | FileCheck %s

# CHECK: 0 finding(s)

.text
.globl _start
_start:
  ld.b    $t0, $a0, 0
  ext.w.b $t1, $t0
  ld.h    $t2, $a1, 0
  ext.w.h $t2, $t3
  ld.bu   $t4, $a2, 0
  ext.w.b $t4, $t4
  ldptr.d $t5, $a3, 0
  addi.w  $t5, $t5, 0
