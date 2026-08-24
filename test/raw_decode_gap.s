## Continue decoding after merged and separated opaque-word gaps.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64 -filetype=obj %s -o %t.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.o %t.bin
# RUN: loonglint --input-format=raw --arch=loongarch64 %t.bin | FileCheck %s --check-prefix=SUMMARY
# RUN: loonglint --base-address=0x1000 --input-format=raw --arch=loongarch64 %t.bin 2>&1 | FileCheck %s --check-prefix=WARNING

# SUMMARY: findings: 0; skipped words: 3; trailing bytes: 0
# WARNING: loonglint: warning: {{.*}}.bin:<raw>: skipped undecodable words in [0x1004, 0x100c)
# WARNING-NEXT: loonglint: warning: {{.*}}.bin:<raw>: skipped undecodable words in [0x1010, 0x1014)

.text
addi.w $r4, $r4, 1
.word 0xffffffff
.word 0xffffffff
addi.w $r5, $r5, 1
.word 0xffffffff
addi.w $r6, $r6, 1
