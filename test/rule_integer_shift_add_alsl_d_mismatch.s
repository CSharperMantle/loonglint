## Reject nonrepresentable or structurally unsafe shift-plus-add pairs.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64-unknown-linux -filetype=obj %s -o %t.o
# RUN: ld.lld --entry=_start %t.o -o %t.exe
# RUN: loonglint %t.exe | FileCheck %s

# CHECK: 0 finding(s)

.text
.globl _start
_start:
  slli.d $t0, $a0, 0
  add.d $t0, $t0, $a1
  slli.d $t1, $a0, 5
  add.d $t1, $t1, $a1
  slli.d $t2, $a0, 2
  add.d $t3, $t2, $a1
  slli.d $t4, $a0, 2
  add.d $t4, $a1, $a2
  slli.d $t5, $a0, 2
  add.d $t5, $t5, $t5
  slli.w $t6, $a0, 2
  add.w $t6, $t6, $a1
