## Reject relocatable ELF input.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64-unknown-linux -filetype=obj %s -o %t.o
# RUN: not loonglint %t.o 2>&1 | FileCheck %s

# CHECK: loonglint: error: unsupported ELF type in '{{.*}}.o': expected ET_EXEC or ET_DYN

.text
addi.w $a0, $a0, 1
