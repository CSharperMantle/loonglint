## Find direct non-linking branches to the next instruction.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64 -filetype=obj %s -o %t.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.o %t.bin
# RUN: not loonglint --input-format=raw --arch=loongarch64 %t.bin | FileCheck %s

# CHECK: {{.*}}.bin:<raw>:0x0: control/branch-to-next: delete branch to next instruction
# CHECK-NEXT: {{.*}}.bin:<raw>:0x4: control/branch-to-next: delete branch to next instruction
# CHECK-NEXT: findings: 2; skipped words: 0; trailing bytes: 0

.text
b 1f
1:
beq $r4, $r5, 2f
2:
bl 3f
3:
jirl $r0, $r0, 16
addi.w $r6, $r6, 1
b 4f
addi.w $r7, $r7, 1
4:
addi.w $r8, $r8, 1
