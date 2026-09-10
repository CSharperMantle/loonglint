## Match zext.w formation from the slli 32/srli 32 pair.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=riscv64 -mattr=+m,+a,+f,+d,+c,+zba,+zbb,+zbs -filetype=obj %s -o %t.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.o %t.bin
# RUN: not loonglint --input-format=raw --arch=rv64gc_zba_zbb_zbs %t.bin | FileCheck %s

# CHECK-COUNT-1: [riscv:integer/zext-w-form]
# CHECK: 1 finding(s)
# CHECK: 1 riscv:integer/zext-w-form

.text
.globl _start
_start:
.option push
.option norvc
  slli   a0, a1, 32
  srli   a0, a0, 32
.option pop
