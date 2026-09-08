## Continue decoding after merged and separated opaque-word gaps.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64 -filetype=obj %s -o %t.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.o %t.bin
# RUN: loonglint --base-address=0x1000 --input-format=raw --arch=loongarch64 %t.bin 2>&1 | FileCheck %s

# CHECK: loonglint: warning: {{.*}}.bin:<raw>: skipped undecodable words in [0x1004, 0x100c)
# CHECK-NEXT: loonglint: warning: {{.*}}.bin:<raw>: skipped undecodable words in [0x1010, 0x1014)

.text
addi.w $a0, $a0, 1
.word 0xffffffff
.word 0xffffffff
addi.w $a1, $a1, 1
.word 0xffffffff
addi.w $a2, $a2, 1
