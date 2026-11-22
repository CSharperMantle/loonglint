## Match adjacent logic-immediate pairs; the andi arm also trips NopRule.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=riscv64 -mattr=+c -filetype=obj %s -o %t.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.o %t.bin
# RUN: not loonglint --filter '^riscv:integer/logic-immediate$' --input-format=raw --arch=rv64gc %t.bin | FileCheck %s

# CHECK-COUNT-4: [riscv:integer/logic-immediate]
# CHECK: 4 finding(s)
# CHECK: 4 riscv:integer/logic-immediate

.text
.globl _start
_start:
.option push
.option norvc
  ori  a0, a1, 1
  ori  a0, a0, 2
  xori a1, a2, 5
  xori a1, a1, 5
  xori a2, a2, 5
  xori a2, a2, 5
  andi a3, a4, -1
  andi a3, a3, -1
.option pop
