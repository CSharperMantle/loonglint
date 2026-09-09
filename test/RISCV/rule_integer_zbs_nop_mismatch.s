## Reject different bit indices, operations, and destinations.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=riscv64 -mattr=+m,+a,+f,+d,+c,+zba,+zbb,+zbs -filetype=obj %s -o %t.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.o %t.bin
# RUN: loonglint --input-format=raw --arch=rv64gc_zba_zbb_zbs %t.bin | FileCheck %s

# CHECK: 0 finding(s)

.text
.globl _start
_start:
.option push
.option norvc
  bclri a0, a1, 3
  bclri a0, a0, 4
  bseti a1, a2, 5
  bclri a1, a1, 5
  binvi a2, a3, 7
  binvi a4, a2, 7
.option pop
