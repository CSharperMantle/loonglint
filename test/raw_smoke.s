## Decode explicit and auto-detected raw LA32 and LA64 input.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch32 -filetype=obj %s -o %t.32.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.32.o %t.32.bin
# RUN: loonglint --input-format=raw --arch=loongarch32 %t.32.bin | FileCheck %s --check-prefix=SUCCESS

# RUN: llvm-mc -triple=loongarch64 -filetype=obj %s -o %t.64.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.64.o %t.64.bin
# RUN: loonglint --input-format=raw --arch=loongarch64 %t.64.bin | FileCheck %s --check-prefix=SUCCESS
# RUN: loonglint --arch=loongarch64 %t.64.bin | FileCheck %s --check-prefix=SUCCESS
# RUN: not loonglint --input-format=raw %t.64.bin 2>&1 | FileCheck %s --check-prefix=EXPLICIT-NO-ARCH
# RUN: not loonglint %t.64.bin 2>&1 | FileCheck %s --check-prefix=AUTO-NO-ARCH

# SUCCESS: 0 finding(s)
# EXPLICIT-NO-ARCH: loonglint: error: --arch is required with --input-format=raw
# AUTO-NO-ARCH: loonglint: error: --arch is required for raw input

.text
addi.w $a0, $a0, 1
