## Reject broken chains, aliasing, self-inverted sources, and non-inverted
## masks.
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
  xori t0, a2, -1
  and  a0, a1, t0
  nop
  xori t2, a4, -2
  and  t2, a5, t2
  nop
  xori t3, t3, -1
  and  t3, a6, t3
.option pop
