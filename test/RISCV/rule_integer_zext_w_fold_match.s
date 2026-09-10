## Match folding zext.w into its .uw consumer.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=riscv64 -mattr=+m,+a,+f,+d,+c,+zba,+zbb,+zbs -filetype=obj %s -o %t.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.o %t.bin
# RUN: not loonglint --input-format=raw --arch=rv64gc_zba_zbb_zbs %t.bin | FileCheck %s

# CHECK-COUNT-3: [riscv:integer/zext-w-fold]
# CHECK: 3 finding(s)
# CHECK: 3 riscv:integer/zext-w-fold

.text
.globl _start
_start:
.option push
.option norvc
  zext.w t0, a5
  add    t0, t0, a6
  zext.w t1, a6
  sh1add t1, t1, a7
  zext.w t2, a7
  slli   t2, t2, 3
.option pop
