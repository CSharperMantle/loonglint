## Sort nonzero rule hit counts by descending count.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64 -filetype=obj %s -o %t.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.o %t.bin
# RUN: not loonglint --color=false --input-format=raw --arch=loongarch64 %t.bin | FileCheck %s

# CHECK: 3 finding(s)
# CHECK-NEXT:	control/branch-to-next: 2
# CHECK-NEXT:	integer/self-move: 1
# CHECK-NOT: Scan incomplete

.text
b 1f
1:
beq $r4, $r5, 2f
2:
or $r6, $r6, $r0
