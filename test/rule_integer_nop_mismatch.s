## Reject non-identity logical forms.
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
  or    $a0, $a1, $zero
  or    $a2, $zero, $a3
  and   $a4, $a4, $a5
  andn  $a6, $zero, $a6
  xor   $a7, $a7, $a7
  ori   $t0, $t0, 1
  xori  $t1, $t2, 0
