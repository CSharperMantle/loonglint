## Reject non-identity .W forms on LA32; .W-by-0 is not an identity on LA64.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch32-unknown-linux -filetype=obj %s -o %t.32.o
# RUN: ld.lld --entry=_start %t.32.o -o %t.32.exe
# RUN: loonglint %t.32.exe | FileCheck %s

# RUN: llvm-mc -triple=loongarch64-unknown-linux -filetype=obj %s -o %t.64.o
# RUN: ld.lld --entry=_start %t.64.o -o %t.64.exe
# RUN: loonglint %t.64.exe | FileCheck %s

# CHECK: 0 finding(s)

.text
.globl _start
_start:
  add.w   $a0, $a1, $zero
  addi.w  $a2, $a2, 1
  slli.w  $a3, $a4, 0
