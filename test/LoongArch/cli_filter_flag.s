## Filter rules via repeated --filter/-F flags.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64 -filetype=obj %s -o %t.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.o %t.bin

# RUN: not loonglint --input-format=raw --arch=loongarch64 --filter integer/nop %t.bin | FileCheck %s --check-prefix=INCLUDE-INTEGER
# INCLUDE-INTEGER: {{.*}}.bin:<raw>:0x4: delete or replace non-canonical NOP instruction [loongarch:integer/nop]
# INCLUDE-INTEGER-NEXT:   - 0x00000004  move $a0, $a0
# INCLUDE-INTEGER-EMPTY:
# INCLUDE-INTEGER-NEXT: 1 finding(s)
# INCLUDE-INTEGER-NEXT: 1 loongarch:integer/nop
# INCLUDE-INTEGER-NOT: branch-to-next
# INCLUDE-INTEGER-NOT: Scan incomplete

# RUN: not loonglint --input-format=raw --arch=loongarch64 --filter control/ %t.bin | FileCheck %s --check-prefix=INCLUDE-CONTROL
# INCLUDE-CONTROL: {{.*}}.bin:<raw>:0x0: delete branch to next instruction [loongarch:control/branch-to-next]
# INCLUDE-CONTROL-NEXT:   - 0x00000000  b 4
# INCLUDE-CONTROL-EMPTY:
# INCLUDE-CONTROL-NEXT: 1 finding(s)
# INCLUDE-CONTROL-NEXT: 1 loongarch:control/branch-to-next
# INCLUDE-CONTROL-NOT: integer/nop

# RUN: not loonglint --input-format=raw --arch=loongarch64 --filter control/ --filter integer/ %t.bin | FileCheck %s --check-prefix=INCLUDE-UNION
# INCLUDE-UNION: {{.*}}.bin:<raw>:0x0: delete branch to next instruction [loongarch:control/branch-to-next]
# INCLUDE-UNION-NEXT:   - 0x00000000  b 4
# INCLUDE-UNION-EMPTY:
# INCLUDE-UNION-NEXT: {{.*}}.bin:<raw>:0x4: delete or replace non-canonical NOP instruction [loongarch:integer/nop]
# INCLUDE-UNION-NEXT:   - 0x00000004  move $a0, $a0
# INCLUDE-UNION-EMPTY:
# INCLUDE-UNION-NEXT: 2 finding(s)
# INCLUDE-UNION-NEXT: 1 loongarch:integer/nop
# INCLUDE-UNION-NEXT: 1 loongarch:control/branch-to-next

# RUN: not loonglint --input-format=raw --arch=loongarch64 -F '^loongarch:(control|integer)/' %t.bin | FileCheck %s --check-prefix=INCLUDE-CATEGORIES
# INCLUDE-CATEGORIES: 2 finding(s)
# INCLUDE-CATEGORIES-NEXT: 1 loongarch:integer/nop
# INCLUDE-CATEGORIES-NEXT: 1 loongarch:control/branch-to-next

.text
b 1f
1:
or $a0, $a0, $zero
