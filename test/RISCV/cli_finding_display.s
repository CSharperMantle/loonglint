## Render kept regions with both original and patched addresses; the unchanged
## instruction is shown once instead of duplicating across removed/added lines.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc --triple=riscv32-unknown-elf -mattr=+c,+a -filetype=obj %s -o %t.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.o %t.bin
# RUN: not loonglint --input-format=raw --arch=rv32imac --filter '^riscv:integer/zero-consumer$' %t.bin | FileCheck %s

# CHECK: {{.*}}.bin:<raw>:0x0: replace consumers of the materialized 0 with register 'zero' [riscv:integer/zero-consumer]
# CHECK-NEXT:     0x00000000  0x00000000  li a0, 0
# CHECK-NEXT:   - 0x00000004              sb a0, 0(a1)
# CHECK-NEXT:   +             0x00000004  sb zero, 0(a1)
# CHECK-EMPTY:
# CHECK-NEXT: {{.*}}.bin:<raw>:0x8: replace consumers of the materialized 0 with register 'zero' [riscv:integer/zero-consumer]
# CHECK-NEXT:     0x00000008  0x00000008  li a3, 0
# CHECK-NEXT:   - 0x0000000c              bne a3, a4, 0x10
# CHECK-NEXT:   +             0x0000000c  bne zero, a4, 0x10
# CHECK-EMPTY:
# CHECK-NEXT: {{.*}}.bin:<raw>:0x10: replace consumers of the materialized 0 with register 'zero' [riscv:integer/zero-consumer]
# CHECK-NEXT:     0x00000010  0x00000010  li t0, 0
# CHECK-NEXT:   - 0x00000012              bne t0, t1, 0x16
# CHECK-NEXT:   +             0x00000012  bne zero, t1, 0x16
# CHECK-EMPTY:
# CHECK-NEXT: 3 finding(s)
# CHECK-NEXT: 3 riscv:integer/zero-consumer
# CHECK-NOT: Scan incomplete

.option push
.option norvc
addi a0, zero, 0
sb a0, 0(a1)
mv a3, zero
bne a3, a4, 1f
1:
.option pop
mv t0, zero
bne t0, t1, 1f
1:
