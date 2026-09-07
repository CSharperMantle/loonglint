# SPDX-License-Identifier: GPL-3.0-or-later

if(NOT DEFINED LLVM_MAIN_SRC_DIR OR NOT COMMAND add_llvm_tool)
  message(FATAL_ERROR
    "LoongLint must be built through LLVM_EXTERNAL_PROJECTS against an LLVM source tree")
endif()

if(NOT "LoongArch" IN_LIST LLVM_TARGETS_TO_BUILD)
  message(FATAL_ERROR "LoongLint requires LoongArch in LLVM_TARGETS_TO_BUILD")
endif()

set(LLVM_SUBPROJECT_TITLE "loonglint")
set(LOONGLINT_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(LOONGLINT_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}")

find_program(LOONGLINT_CLANG_TIDY NAMES clang-tidy REQUIRED)

set(LLVM_LINK_COMPONENTS
  BinaryFormat
  LoongArchDesc
  LoongArchDisassembler
  LoongArchInfo
  MC
  MCDisassembler
  Object
  Support
  TargetParser
)

add_llvm_library(LoongLint
  STATIC
  DISABLE_PCH_REUSE
  src/DisassemblerTarget.cpp
  src/LoongArchSpec.cpp
  src/RuleFilter.cpp
  src/RuleManager.cpp
  src/Rules/LoongArch/AddiPairRule.cpp
  src/Rules/LoongArch/AddressLoadRule.cpp
  src/Rules/LoongArch/AndNotRule.cpp
  src/Rules/LoongArch/BitCountRule.cpp
  src/Rules/LoongArch/BitExtractRule.cpp
  src/Rules/LoongArch/BitReverseRule.cpp
  src/Rules/LoongArch/BranchToNextRule.cpp
  src/Rules/LoongArch/ByteReverseRule.cpp
  src/Rules/LoongArch/DegenerateBranchRule.cpp
  src/Rules/LoongArch/IndexedLoadRule.cpp
  src/Rules/LoongArch/LoadExtendRule.cpp
  src/Rules/LoongArch/LoadZeroExtendRule.cpp
  src/Rules/LoongArch/MulhSextRule.cpp
  src/Rules/LoongArch/NopLA32Rule.cpp
  src/Rules/LoongArch/NopLA64Rule.cpp
  src/Rules/LoongArch/NopRule.cpp
  src/Rules/LoongArch/NotOrRule.cpp
  src/Rules/LoongArch/OrNotRule.cpp
  src/Rules/LoongArch/RotateCombineRule.cpp
  src/Rules/LoongArch/ShiftAddAlslDRule.cpp
  src/Rules/LoongArch/ShiftChainRule.cpp
  src/Rules/LoongArch/ShiftDoubleRule.cpp
  src/Rules/LoongArch/ShiftMaskRule.cpp
  src/Rules/LoongArch/UnsignedLoadPickRule.cpp
  src/Rules/LoongArch/ZeroExtendRule.cpp
  src/ScannedRegion.cpp
)

set_target_properties(LoongLint PROPERTIES
  CXX_EXTENSIONS OFF
  CXX_STANDARD 20
  CXX_STANDARD_REQUIRED ON
  CXX_CLANG_TIDY "${LOONGLINT_CLANG_TIDY}"
)

target_include_directories(LoongLint PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_include_directories(LoongLint SYSTEM PRIVATE
  "${LLVM_MAIN_SRC_DIR}/lib/Target/LoongArch"
  "${LLVM_BINARY_DIR}/lib/Target/LoongArch"
)

add_dependencies(LoongLint LoongArchCommonTableGen)

add_llvm_tool(loonglint
  DISABLE_PCH_REUSE
  src/main.cpp
)

set_target_properties(loonglint PROPERTIES
  CXX_EXTENSIONS OFF
  CXX_STANDARD 20
  CXX_STANDARD_REQUIRED ON
  CXX_CLANG_TIDY "${LOONGLINT_CLANG_TIDY}"
)

target_link_libraries(loonglint PRIVATE LoongLint)

target_include_directories(loonglint PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

if(LLVM_INCLUDE_TESTS)
  add_subdirectory(unittests)
  add_subdirectory(test)
endif()
