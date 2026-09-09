## Match repeated single-bit operations.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=riscv64 -mattr=+m,+a,+f,+d,+c,+zba,+zbb,+zbs -filetype=obj %s -o %t.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.o %t.bin
# RUN: not loonglint --input-format=raw --arch=rv64gc_zba_zbb_zbs %t.bin | FileCheck %s

# CHECK-COUNT-4: [riscv:integer/zbs-nop]
# CHECK: 4 finding(s)
# CHECK: 4 riscv:integer/zbs-nop

.text
.globl _start
_start:
.option push
.option norvc
  bclri a0, a1, 3
  bclri a0, a0, 3
  bseti a1, a2, 5
  bseti a1, a1, 5
  binvi a2, a3, 7
  binvi a2, a2, 7
  binvi a3, a3, 9
  binvi a3, a3, 9
.option pop
