# LoongLint

LoongLint (also stylized as "loonglint" and "LOONGLINT" in the source code) checks [LoongArch](https://docs.kernel.org/arch/loongarch/introduction.html) binaries for common optimizable peephole patterns.

## Design

This is a peephole linter for LoongArch{32S,64} ELF and raw binaries. It scans the input binary, linearly, for pre-defined patterns that are often optimizable. For example, the following two-instruction sequences can be fused into one:

```asm
addi.d $t0, $t0, 8
ld.d $t0, $t0, 0

# ->

ld.d $t0, $t0, 8
```

For a list of implemented peephole rules, please refer to [RULES.md](./RULES.md).

Note that due to the intrinsic limitation of linear scan, there might be false positives due to unrecognized entries. Please take the lint results with a grain of salt.

This project is inspired by [gaul/armlint](https://github.com/gaul/armlint). The disassembly and analysis infrastructure is provided by [LLVM](https://llvm.org/). Matching is done by a [BOLT](https://github.com/llvm/llvm-project/blob/main/bolt/README.md)-inspired DSL.

## Optimization showcase

The following is a list of optimization opportunities found with the help of LoongLint. Note that this list is not complete and will be amended from time to time.

### [SpiderMonkey](https://spidermonkey.dev/)

* [Bug 2066681](https://bugzil.la/2066681): Stop generating no-op self addi.d
* [Bug 2066686](https://bugzil.la/2066686): Fuse or32+not32 into NOR in EmitInitDependentStringBase
* [Bug 2067472](https://bugzil.la/2067472): Refactor to use a more idiomatic `move`-ing pattern
* [Bug 2067473](https://bugzil.la/2067473): Optimize BaseIndex-shaped ma_{load,store} with offset == 0 and scale == 1

## Installation

### From source

Get a modern C++ toolchain that supports C++20. If your compiler can build modern LLVM, it's likely usable here as well.

Set the variable `LLVM_PROJECT` to a source clone of <https://github.com/llvm/llvm-project/>. At the time of writing, the developing LLVM version is 24.0.0. LoongLint should also compile with not-too-older trees, like LLVM 23 and 22.

```sh
cd loonglint
cmake -S "$LLVM_PROJECT"/llvm -B build -G Ninja -DLLVM_EXTERNAL_PROJECTS='loonglint' -DLLVM_EXTERNAL_LOONGLINT_SOURCE_DIR="$(pwd)" -DLLVM_TARGETS_TO_BUILD='LoongArch' -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
ninja -C build
```

## License

Copyright (c) 2026 Rong Bao <<rong.bao@csmantle.top>>.

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but **WITHOUT ANY WARRANTY**; without even the implied warranty of **MERCHANTABILITY** or **FITNESS FOR A PARTICULAR PURPOSE**. See the GNU General Public License for more details.

You should have received [a copy](./LICENSE) of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.

Some parts of the program are Derivative Works of the LLVM source. Each of such source is indicated by special comment headers. Please see [LICENSE-LLVM](./LICENSE-LLVM) for licensing information of such content.
