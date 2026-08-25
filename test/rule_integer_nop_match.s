## Match width-independent identity operations in LA32 and LA64 code.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch32-unknown-linux -filetype=obj %s -o %t.32.o
# RUN: ld.lld --entry=_start %t.32.o -o %t.32.exe
# RUN: not loonglint %t.32.exe | FileCheck %s

# RUN: llvm-mc -triple=loongarch64-unknown-linux -filetype=obj %s -o %t.64.o
# RUN: ld.lld --entry=_start %t.64.o -o %t.64.exe
# RUN: not loonglint %t.64.exe | FileCheck %s

# CHECK-COUNT-7: [integer/nop]
# CHECK: 7 finding(s)
# CHECK: 7 integer/nop

.text
.globl _start
_start:
  or    $a0, $a0, $zero
  or    $a1, $a1, $a1
  and   $a2, $a2, $a2
  andn  $a3, $a3, $zero
  xor   $a4, $a4, $zero
  ori   $a5, $a5, 0
  xori  $a6, $a6, 0
  nop
