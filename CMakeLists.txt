# SPDX-License-Identifier: GPL-3.0-or-later

if(NOT DEFINED LLVM_MAIN_SRC_DIR OR NOT COMMAND add_llvm_tool)
  message(FATAL_ERROR
    "loonglint must be built through LLVM_EXTERNAL_PROJECTS against an LLVM source tree")
endif()

if(NOT "LoongArch" IN_LIST LLVM_TARGETS_TO_BUILD)
  message(FATAL_ERROR "loonglint requires LoongArch in LLVM_TARGETS_TO_BUILD")
endif()

set(LLVM_SUBPROJECT_TITLE "loonglint")
set(LOONGLINT_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(LOONGLINT_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}")

set(LLVM_LINK_COMPONENTS
  Support
)

add_llvm_tool(loonglint
  DISABLE_PCH_REUSE
  src/main.cpp
)

set_target_properties(loonglint PROPERTIES
  CXX_EXTENSIONS OFF
  CXX_STANDARD 20
  CXX_STANDARD_REQUIRED ON
)

target_include_directories(loonglint PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_include_directories(loonglint SYSTEM PRIVATE
  "${LLVM_MAIN_SRC_DIR}/lib/Target/LoongArch"
  "${LLVM_BINARY_DIR}/lib/Target/LoongArch"
)

add_dependencies(loonglint LoongArchCommonTableGen)

if(LLVM_INCLUDE_TESTS)
  add_subdirectory(test)
endif()
