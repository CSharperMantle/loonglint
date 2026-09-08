## Regex match-mode matrix for -E/--exclude: unanchored search, anchors.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64 -filetype=obj %s -o %t.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.o %t.bin

# RUN: not loonglint --input-format=raw --arch=loongarch64 -E branch %t.bin | FileCheck %s --check-prefix=SUBSTR
# SUBSTR: 1 finding(s)
# SUBSTR-NEXT: 1 loongarch:integer/nop
# SUBSTR-NOT: branch-to-next

# RUN: not loonglint --input-format=raw --arch=loongarch64 -E '^loongarch:control/' %t.bin | FileCheck %s --check-prefix=ANCHORED-CATEGORY
# ANCHORED-CATEGORY: 1 finding(s)
# ANCHORED-CATEGORY-NEXT: 1 loongarch:integer/nop
# ANCHORED-CATEGORY-NOT: branch-to-next

# RUN: not loonglint --input-format=raw --arch=loongarch64 -E '^loongarch:integer/nop$' %t.bin | FileCheck %s --check-prefix=ANCHORED-FULL
# ANCHORED-FULL: 1 finding(s)
# ANCHORED-FULL-NEXT: 1 loongarch:control/branch-to-next
# ANCHORED-FULL-NOT: integer/nop

# RUN: not loonglint --input-format=raw --arch=loongarch64 -E '^nop' %t.bin | FileCheck %s --check-prefix=ANCHOR-MISMATCH
# ANCHOR-MISMATCH: 2 finding(s)
# ANCHOR-MISMATCH-NEXT: 1 loongarch:integer/nop
# ANCHOR-MISMATCH-NEXT: 1 loongarch:control/branch-to-next

.text
b 1f
1:
or $a0, $a0, $zero
