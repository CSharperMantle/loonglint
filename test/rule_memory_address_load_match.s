## Fold address ADDI into load displacement.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64-unknown-linux -filetype=obj %s -o %t.o
# RUN: ld.lld --entry=_start %t.o -o %t.exe
# RUN: not loonglint %t.exe | FileCheck %s

# CHECK-COUNT-2: [memory/address-load]
# CHECK: 2 finding(s)
# CHECK: 2 memory/address-load

.text
.globl _start
_start:
  addi.d $t0, $a0, 8
  ld.d   $t0, $t0, 16
  addi.d $t1, $a1, -4
  ld.d   $t1, $t1, 8
