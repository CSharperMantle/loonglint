## Continue decoding after merged and separated opaque-word gaps.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64 -filetype=obj %s -o %t.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.o %t.bin
# RUN: loonglint --input-format=raw --arch=loongarch64 %t.bin | FileCheck %s --check-prefix=SUMMARY
# RUN: loonglint --base-address=0x1000 --input-format=raw --arch=loongarch64 %t.bin 2>&1 | FileCheck %s --check-prefix=WARNING

# SUMMARY: 0 finding(s)
# SUMMARY-NEXT: Scan incomplete: 3 undecodable word(s) skipped; 1 trailing byte(s) ignored.
# WARNING: loonglint: warning: {{.*}}.bin:<raw>: skipped undecodable words in [0x1004, 0x100c)
# WARNING-NEXT: loonglint: warning: {{.*}}.bin:<raw>: skipped undecodable words in [0x1010, 0x1014)
# WARNING-NEXT: loonglint: warning: {{.*}}.bin:<raw>: ignored 1 trailing bytes at 0x1018

.text
addi.w $r4, $r4, 1
.word 0xffffffff
.word 0xffffffff
addi.w $r5, $r5, 1
.word 0xffffffff
addi.w $r6, $r6, 1
.byte 1
