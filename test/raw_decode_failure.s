## Reject input when no word decodes.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64 -filetype=obj %s -o %t.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.o %t.bin
# RUN: not loonglint --color=false --input-format=raw --arch=loongarch64 %t.bin 2>&1 | FileCheck %s

# CHECK: loonglint: error: no instructions decoded from '{{.*}}.bin'

.text
.word 0xffffffff
.word 0xffffffff
