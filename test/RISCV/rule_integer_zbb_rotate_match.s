## Match adjacent rotations, including full-turn identities.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=riscv64 -mattr=+m,+a,+f,+d,+c,+zba,+zbb,+zbs -filetype=obj %s -o %t.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.o %t.bin
# RUN: not loonglint --input-format=raw --arch=rv64gc_zba_zbb_zbs %t.bin | FileCheck %s

# CHECK-COUNT-4: [riscv:integer/zbb-rotate]
# CHECK: 4 finding(s)
# CHECK: 4 riscv:integer/zbb-rotate

.text
.globl _start
_start:
.option push
.option norvc
  rori a0, a1, 3
  rori a0, a0, 4
  rori a1, a2, 20
  rori a1, a1, 50
  rori a2, a3, 40
  rori a2, a2, 24
  rori a3, a3, 40
  rori a3, a3, 24
.option pop
