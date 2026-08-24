## Render original and deletion suggestion in verbose mode.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64 -filetype=obj %s -o %t.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.o %t.bin
# RUN: not loonglint -v --base-address=0x1000 --input-format=raw --arch=loongarch64 %t.bin | FileCheck %s

# CHECK: {{.*}}.bin:raw:0x1000: integer/self-move: delete redundant self-move
# CHECK-NEXT:   original: move $a0, $a0
# CHECK-NEXT:   suggested: <delete>
# CHECK-NEXT: findings: 1; skipped words: 0; trailing bytes: 0

.text
or $r4, $r4, $r0
