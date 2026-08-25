## Match LA64 shift-plus-add pairs representable by ALSL.D.
## SPDX-License-Identifier: GPL-3.0-or-later

# RUN: llvm-mc -triple=loongarch64-unknown-linux -filetype=obj %s -o %t.o
# RUN: ld.lld --entry=_start %t.o -o %t.exe
# RUN: not loonglint %t.exe | FileCheck %s

# CHECK: {{.*}}: fuse shift and add into ALSL.D [integer/shift-add-alsl-d]
# CHECK-NEXT: {{.*}}slli.d{{.*}}$t0, $a0, 1
# CHECK-NEXT: {{.*}}add.d{{.*}}$t0, $t0, $a1
# CHECK-NEXT: {{.*}}alsl.d{{.*}}$t0, $a0, $a1, 1
# CHECK: {{.*}}slli.d{{.*}}$t1, $a2, 4
# CHECK-NEXT: {{.*}}add.d{{.*}}$t1, $a3, $t1
# CHECK-NEXT: {{.*}}alsl.d{{.*}}$t1, $a2, $a3, 4
# CHECK: {{.*}}slli.d{{.*}}$a4, $a4, 2
# CHECK-NEXT: {{.*}}add.d{{.*}}$a4, $a4, $a5
# CHECK-NEXT: {{.*}}alsl.d{{.*}}$a4, $a4, $a5, 2
# CHECK: {{.*}}slli.d{{.*}}$t2, $a6, 3
# CHECK-NEXT: {{.*}}add.d{{.*}}$t2, $t2, $a6
# CHECK-NEXT: {{.*}}alsl.d{{.*}}$t2, $a6, $a6, 3
# CHECK: 4 finding(s)
# CHECK: integer/shift-add-alsl-d: 4

.text
.globl _start
_start:
  slli.d $t0, $a0, 1
  add.d $t0, $t0, $a1
  slli.d $t1, $a2, 4
  add.d $t1, $a3, $t1
  slli.d $a4, $a4, 2
  add.d $a4, $a4, $a5
  slli.d $t2, $a6, 3
  add.d $t2, $t2, $a6
