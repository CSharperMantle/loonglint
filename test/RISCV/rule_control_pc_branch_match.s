## Match auipc + jalr call pairs that fold into jal.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=riscv64 -mattr=+c -filetype=obj %s -o %t.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.o %t.bin
# RUN: not loonglint --input-format=raw --arch=rv64gc %t.bin | FileCheck %s

# CHECK-COUNT-3: [riscv:control/pc-branch]
# CHECK: 3 finding(s)
# CHECK: 3 riscv:control/pc-branch

.text
.globl _start
_start:
.option push
.option norvc
  auipc ra, 1
  jalr  ra, ra, 0
  auipc ra, 2
  jalr  ra, ra, 8
  auipc t0, 3
  jalr  t0, t0, 4
.option pop
