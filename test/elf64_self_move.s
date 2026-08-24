## Report a finding from linked LoongArch64 ELF.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64-unknown-linux -filetype=obj %s -o %t.o
# RUN: ld.lld --entry=_start -Ttext=0x10000 %t.o -o %t.exe
# RUN: not loonglint %t.exe | FileCheck %s

# CHECK: {{.*}}.exe:.text:0x10000: delete redundant self-move [integer/self-move]
# CHECK-NEXT:   - 0x00010000  move $a0, $a0
# CHECK-EMPTY:
# CHECK-NEXT: findings: 1; skipped words: 0; trailing bytes: 0

.text
.globl _start
_start:
  or $r4, $r4, $r0
