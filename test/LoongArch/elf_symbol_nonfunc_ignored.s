## Ignore non-function symbols and zero-size function symbols for annotation.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64-unknown-linux -filetype=obj %s -o %t.o
# RUN: ld.lld -Ttext=0x20120 --entry=_start %t.o -o %t.exe
# RUN: not loonglint %t.exe 2>&1 | FileCheck %s

# CHECK: <.text>:0x20120: fold address ADDI.[DW] into integer load offset [loongarch:memory/address-load]
# CHECK: 1 finding(s)
# CHECK: 1 loongarch:memory/address-load

.text
.globl _start
_start:
  addi.d $t0, $t0, 8
  ld.d   $t0, $t0, 0

.globl obj_sym
.type obj_sym,@object
obj_sym:
  .size obj_sym, 8

.globl zero_func
.type zero_func,@function
zero_func:
