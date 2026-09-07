# SPDX-License-Identifier: GPL-3.0-or-later

set(LOONGLINT_TEST_DEPENDS_COMMON
  FileCheck
  count
  not
)

set(LOONGLINT_TEST_DEPENDS
  ${LOONGLINT_TEST_DEPENDS_COMMON}
  llvm-mc
  llvm-objcopy
  yaml2obj
)

configure_lit_site_cfg(
  ${CMAKE_CURRENT_SOURCE_DIR}/lit.site.cfg.py.in
  ${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg.py
  MAIN_CONFIG
  ${CMAKE_CURRENT_SOURCE_DIR}/lit.cfg.py
  PATHS
  "LLVM_SOURCE_DIR"
  "LLVM_BINARY_DIR"
  "LLVM_TOOLS_DIR"
  "LOONGLINT_SOURCE_DIR"
  "LOONGLINT_BINARY_DIR"
)

configure_lit_site_cfg(
  ${CMAKE_CURRENT_SOURCE_DIR}/Unit/lit.site.cfg.py.in
  ${CMAKE_CURRENT_BINARY_DIR}/Unit/lit.site.cfg.py
  MAIN_CONFIG
  ${CMAKE_CURRENT_SOURCE_DIR}/Unit/lit.cfg.py
)

add_lit_testsuite(check-loonglint "Running LoongLint regression tests"
  ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS ${LOONGLINT_TEST_DEPENDS} LoongLintUnitTests
)

add_subdirectory(Unit)
