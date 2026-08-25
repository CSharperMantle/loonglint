## Reject non-identity .D forms on LA64.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64-unknown-linux -filetype=obj %s -o %t.o
# RUN: ld.lld --entry=_start %t.o -o %t.exe
# RUN: loonglint %t.exe | FileCheck %s

# CHECK: 0 finding(s)

.text
.globl _start
_start:
  add.d   $a0, $a1, $zero
  addi.d  $a2, $a2, 1
  slli.d  $a3, $a4, 0
