## Render a complete human-readable finding by default.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64 -filetype=obj %s -o %t.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.o %t.bin
# RUN: not loonglint --base-address=0x1000 --input-format=raw --arch=loongarch64 %t.bin | FileCheck %s

# CHECK: {{.*}}.bin:<raw>:0x1000: delete or replace non-canonical NOP instruction [integer/nop]
# CHECK-NEXT:   - 0x00001000  move $a0, $a0
# CHECK-EMPTY:
# CHECK-NEXT: 1 finding(s)
# CHECK-NEXT: 1 integer/nop
# CHECK-NOT: Scan incomplete

.text
or $a0, $a0, $zero
