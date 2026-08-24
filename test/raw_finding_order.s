## Emit findings in address order before using registry order as a tie-breaker.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64 -filetype=obj %s -o %t.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.o %t.bin
# RUN: not loonglint --input-format=raw --arch=loongarch64 %t.bin | FileCheck %s

# CHECK: {{.*}}.bin:<raw>:0x0: delete branch to next instruction [control/branch-to-next]
# CHECK-NEXT:   - 0x00000000  b 4
# CHECK-EMPTY:
# CHECK-NEXT: {{.*}}.bin:<raw>:0x4: delete redundant self-move [integer/self-move]
# CHECK-NEXT:   - 0x00000004  move $a0, $a0
# CHECK-EMPTY:
# CHECK-NEXT: findings: 2; skipped words: 0; trailing bytes: 0

.text
b 1f
1:
or $r4, $r4, $r0
