## --exclude{,-file} and --filter{,-file} are mutually exclusive.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64 -filetype=obj %s -o %t.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.o %t.bin

# RUN: not loonglint --input-format=raw --arch=loongarch64 -E control/ --filter integer/nop %t.bin 2>&1 | FileCheck %s --check-prefix=FLAG-FLAG
# FLAG-FLAG: loonglint: error: --exclude{,-file} and --filter{,-file} are mutually exclusive
# FLAG-FLAG-NOT: finding

# RUN: not loonglint --input-format=raw --arch=loongarch64 --exclude-file %t.missing --filter integer/nop %t.bin 2>&1 | FileCheck %s --check-prefix=FILE-FLAG
# FILE-FLAG: loonglint: error: --exclude{,-file} and --filter{,-file} are mutually exclusive
# FILE-FLAG-NOT: cannot read
# FILE-FLAG-NOT: finding

# RUN: not loonglint --input-format=raw --arch=loongarch64 --filter-file %t.missing -E control/ %t.bin 2>&1 | FileCheck %s --check-prefix=FLAG-FILE
# FLAG-FILE: loonglint: error: --exclude{,-file} and --filter{,-file} are mutually exclusive
# FLAG-FILE-NOT: cannot read
# FLAG-FILE-NOT: finding

.text
b 1f
1:
or $a0, $a0, $zero
