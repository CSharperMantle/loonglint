## Filter rules via --filter-file, its union with --filter/-F, and missing-file error.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64 -filetype=obj %s -o %t.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.o %t.bin

# RUN: printf '  control/  \r\n\r\n# a comment\r\n' > %t.include

# RUN: not loonglint --input-format=raw --arch=loongarch64 --filter-file %t.include %t.bin | FileCheck %s --check-prefix=FILE-ONLY
# FILE-ONLY: 1 finding(s)
# FILE-ONLY-NEXT: 1 loongarch:control/branch-to-next
# FILE-ONLY-NOT: integer/nop

# RUN: not loonglint --input-format=raw --arch=loongarch64 --filter-file %t.include --filter integer/nop %t.bin | FileCheck %s --check-prefix=FILE-UNION
# FILE-UNION: 2 finding(s)
# FILE-UNION-NEXT: 1 loongarch:integer/nop
# FILE-UNION-NEXT: 1 loongarch:control/branch-to-next

# RUN: not loonglint --input-format=raw --arch=loongarch64 --filter-file %t.missing %t.bin 2>&1 | FileCheck %s --check-prefix=FILE-MISSING
# FILE-MISSING: loonglint: error: cannot read file
# FILE-MISSING-NOT: finding

.text
b 1f
1:
or $a0, $a0, $zero
