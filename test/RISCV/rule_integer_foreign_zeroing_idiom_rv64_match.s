## Match the RV64-only W-form foreign zeroing idioms.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=riscv64 -mattr=+m,+a,+f,+d,+c -filetype=obj %s -o %t.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.o %t.bin
# RUN: not loonglint --input-format=raw --arch=rv64gc --filter '^riscv:integer/foreign-zeroing-idiom$' %t.bin | FileCheck %s

# CHECK-COUNT-8: [riscv:integer/foreign-zeroing-idiom]
# CHECK: 8 finding(s)
# CHECK: 8 riscv:integer/foreign-zeroing-idiom

.text
.globl _start
_start:
.option push
.option norvc
  subw   a0, a1, a1
  slliw  a0, zero, 3
  srliw  a0, zero, 3
  sraiw  a0, zero, 3
  sllw   a0, zero, a2
  srlw   a0, zero, a2
  sraw   a0, zero, a2
  addw   a0, zero, zero
  subw   zero, a1, a1
  slliw  a0, a1, 3
  addw   a0, zero, a1
.option pop
