## Filter rules via repeated -E/--exclude flags.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64 -filetype=obj %s -o %t.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.o %t.bin

# RUN: not loonglint --input-format=raw --arch=loongarch64 -E integer/nop %t.bin | FileCheck %s --check-prefix=EXCLUDE-INTEGER
# EXCLUDE-INTEGER: {{.*}}.bin:<raw>:0x0: delete branch to next instruction [control/branch-to-next]
# EXCLUDE-INTEGER-NEXT:   - 0x00000000  b 4
# EXCLUDE-INTEGER-EMPTY:
# EXCLUDE-INTEGER-NEXT: 1 finding(s)
# EXCLUDE-INTEGER-NEXT: 1 control/branch-to-next
# EXCLUDE-INTEGER-NOT: integer/nop
# EXCLUDE-INTEGER-NOT: Scan incomplete

# RUN: not loonglint --input-format=raw --arch=loongarch64 --exclude control/ %t.bin | FileCheck %s --check-prefix=EXCLUDE-CONTROL
# EXCLUDE-CONTROL: {{.*}}.bin:<raw>:0x4: delete or replace non-canonical NOP instruction [integer/nop]
# EXCLUDE-CONTROL-NEXT:   - 0x00000004  move $a0, $a0
# EXCLUDE-CONTROL-EMPTY:
# EXCLUDE-CONTROL-NEXT: 1 finding(s)
# EXCLUDE-CONTROL-NEXT: 1 integer/nop
# EXCLUDE-CONTROL-NOT: branch-to-next

# RUN: loonglint --input-format=raw --arch=loongarch64 -E control/ --exclude integer/ %t.bin | FileCheck %s --check-prefix=EXCLUDE-UNION
# EXCLUDE-UNION: 0 finding(s)
# EXCLUDE-UNION-NOT: [control/
# EXCLUDE-UNION-NOT: [integer/

# RUN: loonglint --input-format=raw --arch=loongarch64 -E '^(control|integer)/' %t.bin | FileCheck %s --check-prefix=EXCLUDE-CATEGORIES
# EXCLUDE-CATEGORIES: 0 finding(s)

.text
b 1f
1:
or $a0, $a0, $zero
