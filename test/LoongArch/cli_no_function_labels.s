## Toggle finding labels between the containing symbol and the section.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64-unknown-linux -filetype=obj %s -o %t.o
# RUN: ld.lld -Ttext=0x20120 --entry=_start %t.o -o %t.exe
# RUN: not loonglint --no-function-labels %t.exe 2>&1 | FileCheck %s --check-prefix=SECTION
# RUN: not loonglint %t.exe 2>&1 | FileCheck %s --check-prefix=SYMBOL

# SECTION: <.text>:0x20120: fold address ADDI.[DW] into integer load offset [loongarch:memory/address-load]
# SECTION: 1 finding(s)
# SECTION: 1 loongarch:memory/address-load
# SYMBOL: _start+0x0: fold address ADDI.[DW] into integer load offset [loongarch:memory/address-load]
# SYMBOL: 1 finding(s)
# SYMBOL: 1 loongarch:memory/address-load

.text
.globl _start
.type _start,@function
_start:
  addi.d $t0, $t0, 8
  ld.d   $t0, $t0, 0
  .size _start, .-_start
