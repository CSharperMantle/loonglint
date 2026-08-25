## Reject non-fusable shift chains.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64-unknown-linux -filetype=obj %s -o %t.o
# RUN: ld.lld --entry=_start %t.o -o %t.exe
# RUN: loonglint %t.exe | FileCheck %s

# CHECK: 0 finding(s)

.text
.globl _start
_start:
  slli.d $t0, $a0, 2
  srli.d $t0, $t0, 3
  slli.d $t1, $a0, 2
  slli.d $t2, $t1, 3
  slli.d $t3, $a0, 30
  slli.d $t3, $t3, 40
