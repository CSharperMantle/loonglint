## Find redundant OR self-moves in LA32 and LA64 code.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch32 -filetype=obj %s -o %t.32.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.32.o %t.32.bin
# RUN: not loonglint --input-format=raw --arch=loongarch32 %t.32.bin | FileCheck %s

# RUN: llvm-mc -triple=loongarch64 -filetype=obj %s -o %t.64.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.64.o %t.64.bin
# RUN: not loonglint --input-format=raw --arch=loongarch64 %t.64.bin | FileCheck %s

# CHECK: {{.*}}.bin:raw:0x0: integer/self-move: delete redundant self-move
# CHECK-NEXT: {{.*}}.bin:raw:0x4: integer/self-move: delete redundant self-move
# CHECK-NEXT: findings: 2; skipped words: 0; trailing bytes: 0

.text
or $r4, $r4, $r0
or $r5, $r0, $r5
or $r6, $r7, $r0
or $r8, $r0, $r9
addi.w $r10, $r10, 1
