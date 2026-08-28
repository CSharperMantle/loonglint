## Reject non-redundant picks after unsigned loads.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64-unknown-linux -filetype=obj %s -o %t.o
# RUN: ld.lld --entry=_start %t.o -o %t.exe
# RUN: loonglint %t.exe | FileCheck %s

# CHECK: 0 finding(s)

.text
.globl _start
_start:
  ld.bu      $t0, $a0, 0
  bstrpick.d $t0, $t0, 63, 0
  ld.hu      $t1, $a1, 0
  bstrpick.d $t1, $t1, 15, 1
  ld.wu      $t2, $a2, 0
  bstrpick.d $t3, $t2, 31, 0
  ld.bu      $t4, $a3, 0
  bstrins.d  $t4, $t4, 7, 0
  add.d      $t5, $a4, $a5
  ld.bu      $t6, $t5, 0
  bstrpick.d $t6, $t6, 15, 0
