## Reject empty raw input.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: touch %t.bin
# RUN: not loonglint --input-format=raw --arch=loongarch64 %t.bin 2>&1 | FileCheck %s

# CHECK: loonglint: error: input '{{.*}}.bin' is empty
