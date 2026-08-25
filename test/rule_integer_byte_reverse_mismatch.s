## Reject non-composing halfword/byte reversal pairs.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64-unknown-linux -filetype=obj %s -o %t.o
# RUN: ld.lld --entry=_start %t.o -o %t.exe
# RUN: loonglint %t.exe | FileCheck %s

# CHECK: 0 finding(s)

.text
.globl _start
_start:
  revb.4h $t0, $a0
  revh.d  $t1, $t0
  revb.4h $t2, $a0
  revb.2w $t2, $t2
