# SPDX-License-Identifier: GPL-3.0-or-later

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

add_lit_testsuite(check-loonglint "Running loonglint regression tests"
  ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS
  FileCheck
  llvm-mc
  llvm-objcopy
  not
  yaml2obj
  loonglint
)
