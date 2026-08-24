## Report bytes that cannot form a complete instruction.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64 -filetype=obj %s -o %t.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.o %t.bin
# RUN: loonglint --input-format=raw --arch=loongarch64 %t.bin | FileCheck %s --check-prefix=SUMMARY
# RUN: loonglint -v --input-format=raw --arch=loongarch64 %t.bin 2>&1 | FileCheck %s --check-prefix=VERBOSE

# SUMMARY: findings: 0; skipped words: 0; trailing bytes: 3
# VERBOSE: loonglint: warning: {{.*}}.bin:<raw>: ignored 3 trailing bytes at 0x4

.text
addi.w $r4, $r4, 1
.byte 1, 2, 3
