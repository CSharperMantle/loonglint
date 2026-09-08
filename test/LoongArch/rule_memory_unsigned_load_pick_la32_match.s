## Delete redundant zero-extension picks after unsigned loads (LA32).
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch32-unknown-linux -filetype=obj %s -o %t.o
# RUN: ld.lld --entry=_start %t.o -o %t.exe
# RUN: not loonglint %t.exe | FileCheck %s

## Covers LD.{BU,HU} followed by the matching BSTRPICK.W.
# CHECK-COUNT-2: [memory/unsigned-load-pick]
# CHECK: 2 finding(s)
# CHECK: 2 memory/unsigned-load-pick

.text
.globl _start
_start:
  ld.bu      $t0, $a0, 0
  bstrpick.w $t0, $t0, 7, 0
  ld.hu      $t1, $a1, 0
  bstrpick.w $t1, $t1, 15, 0
