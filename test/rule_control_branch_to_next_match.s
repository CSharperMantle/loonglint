## Match direct non-linking branches to the next instruction.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64-unknown-linux -filetype=obj %s -o %t.o
# RUN: ld.lld --entry=_start %t.o -o %t.exe
# RUN: not loonglint %t.exe | FileCheck %s

# CHECK-COUNT-2: [control/branch-to-next]

.text
.globl _start
_start:
  b 1f
1:
  beq $a0, $a1, 2f
2:
  addi.w $a2, $a2, 1
