## Decode linked ET_DYN input.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64-unknown-linux -filetype=obj %s -o %t.o
# RUN: ld.lld -shared %t.o -o %t.so
# RUN: loonglint %t.so | FileCheck %s

# CHECK: 0 finding(s)
# CHECK-NOT: Scan incomplete

.text
.globl entry
entry:
  addi.w $r4, $r4, 1
