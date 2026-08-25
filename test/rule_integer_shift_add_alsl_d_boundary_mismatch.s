## Reject shift-plus-add pairs split by control-flow or region boundaries.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64-unknown-linux -filetype=obj %s -o %t.o
# RUN: ld.lld --entry=_start %t.o -o %t.exe
# RUN: loonglint %t.exe 2>&1 | FileCheck %s

# CHECK: 0 finding(s)

.text
.globl _start
_start:
  ## Direct branch targets consumer, so producer/consumer pair crosses entry.
  b 1f
  slli.d $t0, $a0, 2
1:
  add.d $t0, $t0, $a1

  ## Recovered indirect target likewise lands on consumer.
  pcaddu18i $t1, 0
  jirl $zero, $t1, 12
  slli.d $t2, $a2, 2
  add.d $t2, $t2, $a3

  ## Call target lands on consumer, so producer/consumer pair crosses call boundary.
  bl 2f
  slli.d $t4, $a6, 2
2:
  add.d $t4, $t4, $a7

  ## Terminator target likewise lands on consumer.
  b 3f
  slli.d $t5, $a0, 2
3:
  add.d $t5, $t5, $a1
  addi.d $a2, $a2, 1

callee:
  jirl $zero, $ra, 0

.section .init,"ax",@progbits
  slli.d $t6, $a0, 2

.section .fini,"ax",@progbits
  add.d $t6, $t6, $a1
