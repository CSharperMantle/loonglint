## Reject non-foldable address-load pairs.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64-unknown-linux -filetype=obj %s -o %t.o
# RUN: ld.lld --entry=_start %t.o -o %t.exe
# RUN: loonglint %t.exe | FileCheck %s

## Includes the deferred LA64 .W form.
# CHECK: 0 finding(s)

.text
.globl _start
_start:
  addi.d $t0, $a0, 8
  ld.d   $t1, $t0, 16
  addi.w $t2, $a0, 8
  ld.w   $t2, $t2, 16
