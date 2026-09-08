## Delete redundant zero-extension picks after unsigned loads (LA64).
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64-unknown-linux -filetype=obj %s -o %t.o
# RUN: ld.lld --entry=_start %t.o -o %t.exe
# RUN: not loonglint %t.exe | FileCheck %s

## Covers LD.{BU,HU,WU} and LDX.{BU,HU,WU} followed by the matching BSTRPICK.D.
# CHECK-COUNT-6: [memory/unsigned-load-pick]
# CHECK: 6 finding(s)
# CHECK: 6 memory/unsigned-load-pick

.text
.globl _start
_start:
  ld.bu      $t0, $a0, 0
  bstrpick.d $t0, $t0, 7, 0
  ld.hu      $t1, $a1, 0
  bstrpick.d $t1, $t1, 15, 0
  ld.wu      $t2, $a2, 0
  bstrpick.d $t2, $t2, 31, 0
  add.d      $t3, $a3, $a4
  ldx.bu     $t3, $t3, $zero
  bstrpick.d $t3, $t3, 7, 0
  add.d      $t4, $a3, $a4
  ldx.hu     $t4, $t4, $zero
  bstrpick.d $t4, $t4, 15, 0
  add.d      $t5, $a3, $a4
  ldx.wu     $t5, $t5, $zero
  bstrpick.d $t5, $t5, 31, 0
