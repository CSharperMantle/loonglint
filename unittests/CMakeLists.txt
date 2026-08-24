# SPDX-License-Identifier: GPL-3.0-or-later

add_custom_target(LoongLintUnitTests)
set_target_properties(LoongLintUnitTests PROPERTIES FOLDER "LoongLint/Tests")

set(LLVM_LINK_COMPONENTS
  MC
  Support
)

add_unittest(LoongLintUnitTests LoongLintCoreTests
  DISABLE_PCH_REUSE
  MCInstMatcherTest.cpp
)

set_target_properties(LoongLintCoreTests PROPERTIES
  CXX_EXTENSIONS OFF
  CXX_STANDARD 20
  CXX_STANDARD_REQUIRED ON
)

target_include_directories(LoongLintCoreTests PRIVATE
  "${LOONGLINT_SOURCE_DIR}/include"
)

set_property(TARGET LoongLintCoreTests PROPERTY
  CXX_CLANG_TIDY "${LOONGLINT_CLANG_TIDY}"
)
