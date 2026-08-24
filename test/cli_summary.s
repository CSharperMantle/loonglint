## Render sorted rule counts and scan-incompleteness notes.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64 -filetype=obj %s -o %t.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.o %t.bin
# RUN: not loonglint --input-format=raw --arch=loongarch64 %t.bin | FileCheck %s

# CHECK: 3 finding(s)
# CHECK-NEXT:	control/branch-to-next: 2
# CHECK-NEXT:	integer/self-move: 1
# CHECK-NEXT: Scan incomplete: 1 undecodable word(s) skipped; 1 trailing byte(s) ignored.

.text
b 1f
1:
beq $a0, $a1, 2f
2:
or $a2, $a2, $zero
.word 0xffffffff
.byte 1
