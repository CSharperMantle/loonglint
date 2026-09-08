## Decode explicit raw RV64 input through a RISC-V arch-string profile.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=riscv64 -mattr=+c -filetype=obj %s -o %t.64.o
# RUN: llvm-objcopy -O binary --only-section=.text %t.64.o %t.64.bin
# RUN: not loonglint --input-format=raw --arch=rv64gc %t.64.bin | FileCheck %s --check-prefix=SUCCESS
# RUN: not loonglint --input-format=raw --arch=rv64gg %t.64.bin 2>&1 | FileCheck %s --check-prefix=BAD-ARCH

# SUCCESS: 1 finding(s)
# SUCCESS: 1 riscv:integer/nop
# BAD-ARCH: loonglint: error: unknown architecture 'rv64gg'

.text
c.addi a0, 0
