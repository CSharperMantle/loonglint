## Reject empty raw input.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64 -filetype=obj %s -o %t.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.o %t.bin
# RUN: not loonglint --input-format=raw --arch=loongarch64 %t.bin 2>&1 | FileCheck %s

# CHECK: loonglint: error: input '{{.*}}.bin' is empty

.text
