## Regex match-mode matrix for --filter/-F: unanchored search, anchors.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64 -filetype=obj %s -o %t.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.o %t.bin

# RUN: not loonglint --input-format=raw --arch=loongarch64 --filter branch %t.bin | FileCheck %s --check-prefix=SUBSTR
# SUBSTR: 1 finding(s)
# SUBSTR-NEXT: 1 loongarch:control/branch-to-next
# SUBSTR-NOT: integer/nop

# RUN: not loonglint --input-format=raw --arch=loongarch64 --filter '^loongarch:integer/' %t.bin | FileCheck %s --check-prefix=ANCHORED-CATEGORY
# ANCHORED-CATEGORY: 1 finding(s)
# ANCHORED-CATEGORY-NEXT: 1 loongarch:integer/nop
# ANCHORED-CATEGORY-NOT: branch-to-next

# RUN: not loonglint --input-format=raw --arch=loongarch64 --filter '^loongarch:control/branch-to-next$' %t.bin | FileCheck %s --check-prefix=ANCHORED-FULL
# ANCHORED-FULL: 1 finding(s)
# ANCHORED-FULL-NEXT: 1 loongarch:control/branch-to-next
# ANCHORED-FULL-NOT: integer/nop

# RUN: loonglint --input-format=raw --arch=loongarch64 --filter '^nop' %t.bin | FileCheck %s --check-prefix=ANCHOR-MISMATCH
# ANCHOR-MISMATCH: 0 finding(s)
# ANCHOR-MISMATCH-NOT: [loongarch:

.text
b 1f
1:
or $a0, $a0, $zero
