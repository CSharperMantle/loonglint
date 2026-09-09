## Ignore function symbols outside executable sections and beyond all sections.
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

.data
.globl data_func
.type data_func,@function
data_func:
  .quad 0
  .size data_func, 8

.globl far_func
far_func = _start + 0x800000
.type far_func,@function
