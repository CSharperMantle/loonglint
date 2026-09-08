## Reject non-zero-extending shift pairs.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64-unknown-linux -filetype=obj %s -o %t.o
# RUN: ld.lld --entry=_start %t.o -o %t.exe
# RUN: loonglint %t.exe | FileCheck %s

# CHECK: 0 finding(s)

.text
.globl _start
_start:
  slli.d $t0, $a0, 56
  srli.d $t1, $t1, 56
  slli.d $t2, $a0, 56
  srli.d $t2, $t2, 48
  slli.w $t3, $a0, 24
  srli.d $t3, $t3, 24
