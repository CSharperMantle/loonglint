## Invalid exclusion patterns are hard errors before any scanning.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64 -filetype=obj %s -o %t.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.o %t.bin

# RUN: not loonglint --input-format=raw --arch=loongarch64 -E '[' %t.bin 2>&1 | FileCheck %s --check-prefix=INVALID-REGEX
# INVALID-REGEX: loonglint: error: invalid regular expression '[':
# INVALID-REGEX-NOT: finding

# RUN: not loonglint --input-format=raw --arch=loongarch64 -E control/ -E '[' %t.bin 2>&1 | FileCheck %s --check-prefix=SECOND-INVALID
# SECOND-INVALID: loonglint: error: invalid regular expression '[':

# RUN: not loonglint --input-format=raw --arch=loongarch64 -E '' %t.bin 2>&1 | FileCheck %s --check-prefix=EMPTY-REGEX
# EMPTY-REGEX: loonglint: error: empty exclusion pattern
# EMPTY-REGEX-NOT: finding

.text
b 1f
1:
or $a0, $a0, $zero
