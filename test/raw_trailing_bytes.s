## Report bytes that cannot form a complete raw instruction.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64 -filetype=obj %s -o %t.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.o %t.bin
# RUN: loonglint --input-format=raw --arch=loongarch64 %t.bin 2>&1 | FileCheck %s

# CHECK: loonglint: warning: {{.*}}.bin:<raw>: ignored 3 trailing bytes at 0x4

.text
addi.w $a0, $a0, 1
.byte 1, 2, 3
