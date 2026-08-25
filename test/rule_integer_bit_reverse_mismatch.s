## Reject non-composing byte/bit reversal pairs.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64-unknown-linux -filetype=obj %s -o %t.o
# RUN: ld.lld --entry=_start %t.o -o %t.exe
# RUN: loonglint %t.exe | FileCheck %s

# CHECK: 0 finding(s)

.text
.globl _start
_start:
  revb.2w  $t0, $a0
  bitrev.w $t1, $t0
  revb.2w  $t2, $a0
  bitrev.d $t2, $t2
