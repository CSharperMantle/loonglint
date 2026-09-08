## Filter rules via --exclude-file, its union with -E, and missing-file error.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64 -filetype=obj %s -o %t.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.o %t.bin

# RUN: printf '  integer/nop  \r\n\r\n# a comment\r\n' > %t.exclude

# RUN: not loonglint --input-format=raw --arch=loongarch64 --exclude-file %t.exclude %t.bin | FileCheck %s --check-prefix=FILE-ONLY
# FILE-ONLY: 1 finding(s)
# FILE-ONLY-NEXT: 1 control/branch-to-next
# FILE-ONLY-NOT: integer/nop

# RUN: loonglint --input-format=raw --arch=loongarch64 --exclude-file %t.exclude -E control/ %t.bin | FileCheck %s --check-prefix=FILE-UNION
# FILE-UNION: 0 finding(s)

# RUN: not loonglint --input-format=raw --arch=loongarch64 --exclude-file %t.missing %t.bin 2>&1 | FileCheck %s --check-prefix=FILE-MISSING
# FILE-MISSING: loonglint: error: cannot read exclude file
# FILE-MISSING-NOT: finding

.text
b 1f
1:
or $a0, $a0, $zero
