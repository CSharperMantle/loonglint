## Match the RV64I W-form HINTs reserved for future standard use.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=riscv64 -mattr=+m,+a,+f,+d,+c -filetype=obj %s -o %t.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.o %t.bin
# RUN: not loonglint --filter '^riscv:integer/reserved-hint$' --input-format=raw --arch=rv64gc %t.bin | FileCheck %s

# CHECK-COUNT-6: [riscv:integer/reserved-hint]
# CHECK: 6 finding(s)
# CHECK: 6 riscv:integer/reserved-hint

.text
.globl _start
_start:
.option push
.option norvc
  addiw zero, a0, 1
  addw  zero, a0, a1
  subw  zero, a0, a1
  sllw  zero, a0, a1
  srlw  zero, a0, a1
  sraw  zero, a0, a1
.option pop
