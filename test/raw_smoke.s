## Clean raw LoongArch32 and LoongArch64 input.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch32 -filetype=obj %s -o %t.32.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.32.o %t.32.bin
# RUN: loonglint --input-format=raw --arch=loongarch32 %t.32.bin | FileCheck %s

# RUN: llvm-mc -triple=loongarch64 -filetype=obj %s -o %t.64.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.64.o %t.64.bin
# RUN: loonglint --input-format=raw --arch=loongarch64 %t.64.bin | FileCheck %s

# CHECK: findings: 0; skipped words: 0; trailing bytes: 0

.text
addi.w $r4, $r4, 1
