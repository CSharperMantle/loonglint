## Annotate with the nearest preceding containing function symbol and its offset.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64-unknown-linux -filetype=obj %s -o %t.o
# RUN: ld.lld -Ttext=0x20120 --entry=_start %t.o -o %t.exe
# RUN: not loonglint %t.exe 2>&1 | FileCheck %s

# CHECK: second+0x4: fold address ADDI.[DW] into integer load offset [loongarch:memory/address-load]
# CHECK: 1 finding(s)
# CHECK: 1 loongarch:memory/address-load

.text
.globl _start
_start:
  nop
.globl first
.type first,@function
first:
  nop
.globl second
.type second,@function
second:
  nop
  addi.d $t0, $t0, 8
  ld.d   $t0, $t0, 0
  .size second, 16
  .size first, 12
