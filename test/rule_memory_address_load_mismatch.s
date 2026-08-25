## Reject non-foldable address-load pairs.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64-unknown-linux -filetype=obj %s -o %t.o
# RUN: ld.lld --entry=_start %t.o -o %t.exe
# RUN: loonglint %t.exe | FileCheck %s

## Includes distinct-destination, deferred LA64 .W, unencodable LDPTR-offset, and `$zero` temporary forms.
# CHECK: 0 finding(s)

.text
.globl _start
_start:
  addi.d $t0, $a0, 8
  ld.b   $t1, $t0, 16
  addi.w $t2, $a0, 8
  ld.w   $t2, $t2, 16
  addi.d $t3, $a2, 2
  ldptr.d $t3, $t3, 16
  addi.d $zero, $a3, 8
  ld.b   $zero, $zero, 16
