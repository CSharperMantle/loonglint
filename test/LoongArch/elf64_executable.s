## Decode linked LoongArch64 ELF.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64-unknown-linux -filetype=obj %s -o %t.o
# RUN: ld.lld --entry=_start %t.o -o %t.exe
# RUN: loonglint --input-format=elf %t.exe | FileCheck %s

# CHECK: 0 finding(s)

.text
.globl _start
_start:
  addi.w $a0, $a0, 1
