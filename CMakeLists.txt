# SPDX-License-Identifier: GPL-3.0-or-later

if(NOT DEFINED LLVM_MAIN_SRC_DIR OR NOT COMMAND add_llvm_tool)
  message(FATAL_ERROR
    "LoongLint must be built through LLVM_EXTERNAL_PROJECTS against an LLVM source tree")
endif()

set(LOONGLINT_ARCHS_all "LoongArch;RISCV")
set(LOONGLINT_ARCHS_default)
foreach (arch ${LOONGLINT_ARCHS_all})
  if (arch IN_LIST LLVM_TARGETS_TO_BUILD)
    list(APPEND LOONGLINT_ARCHS_default ${arch})
  endif()
endforeach()

set(LOONGLINT_ARCHS "${LOONGLINT_ARCHS_default}"
    CACHE STRING "Architectures for LoongLint to support.")
if (NOT LOONGLINT_ARCHS)
  message(FATAL_ERROR "LoongLint enabled but LOONGLINT_ARCHS is empty")
endif()
foreach (arch ${LOONGLINT_ARCHS})
  if (NOT arch IN_LIST LLVM_TARGETS_TO_BUILD)
    message(FATAL_ERROR "LoongLint arch '${arch}' is not in LLVM_TARGETS_TO_BUILD")
  endif()
  message(STATUS "Targeting ${arch} in LoongLint")
endforeach()

set(LOONGLINT_HAS_LOONGARCH OFF)
if("LoongArch" IN_LIST LOONGLINT_ARCHS)
  set(LOONGLINT_HAS_LOONGARCH ON)
endif()
set(LOONGLINT_HAS_RISCV OFF)
if("RISCV" IN_LIST LOONGLINT_ARCHS)
  set(LOONGLINT_HAS_RISCV ON)
endif()

set(LLVM_SUBPROJECT_TITLE "loonglint")
set(LOONGLINT_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(LOONGLINT_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}")

find_program(LOONGLINT_CLANG_TIDY NAMES clang-tidy REQUIRED)

set(LOONGLINT_ENGINE_SOURCES
  src/DisassemblerTarget.cpp
  src/RuleFilter.cpp
  src/RuleManager.cpp
  src/ScannedRegion.cpp
)

set(LOONGLINT_LOONGARCH_SOURCES
  src/LoongArch/LoongArchSpec.cpp
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
)
set(LOONGLINT_RISCV_SOURCES
  src/RISCV/RISCVSpec.cpp
  src/Rules/RISCV/NopRule.cpp
)

set(LOONGLINT_ARCH_SOURCES)
set(LLVM_LINK_COMPONENTS
  BinaryFormat
  MC
  MCDisassembler
  Object
  Support
  TargetParser
)

if(LOONGLINT_HAS_LOONGARCH)
  list(APPEND LOONGLINT_ARCH_SOURCES ${LOONGLINT_LOONGARCH_SOURCES})
  list(APPEND LLVM_LINK_COMPONENTS LoongArchDesc LoongArchDisassembler LoongArchInfo)
endif()
if(LOONGLINT_HAS_RISCV)
  list(APPEND LOONGLINT_ARCH_SOURCES ${LOONGLINT_RISCV_SOURCES})
  list(APPEND LLVM_LINK_COMPONENTS RISCVDesc RISCVDisassembler RISCVInfo)
endif()

add_llvm_library(LoongLint
  STATIC
  DISABLE_PCH_REUSE
  ${LOONGLINT_ENGINE_SOURCES}
  ${LOONGLINT_ARCH_SOURCES}
)

set_target_properties(LoongLint PROPERTIES
  CXX_EXTENSIONS OFF
  CXX_STANDARD 20
  CXX_STANDARD_REQUIRED ON
  CXX_CLANG_TIDY "${LOONGLINT_CLANG_TIDY}"
)

target_include_directories(LoongLint PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
  "${CMAKE_CURRENT_BINARY_DIR}/include"
)

if(LOONGLINT_HAS_LOONGARCH)
  target_include_directories(LoongLint SYSTEM PRIVATE
    "${LLVM_MAIN_SRC_DIR}/lib/Target/LoongArch"
    "${LLVM_BINARY_DIR}/lib/Target/LoongArch"
  )
  add_dependencies(LoongLint LoongArchCommonTableGen)
endif()
if(LOONGLINT_HAS_RISCV)
  target_include_directories(LoongLint SYSTEM PRIVATE
    "${LLVM_MAIN_SRC_DIR}/lib/Target/RISCV"
    "${LLVM_BINARY_DIR}/lib/Target/RISCV"
  )
  add_dependencies(LoongLint RISCVCommonTableGen)
endif()

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
  "${CMAKE_CURRENT_BINARY_DIR}/include"
)

set(LOONGLINT_ENUM_ARCHS "")
set(LOONGLINT_ARCH_INCLUDES "")
foreach(arch ${LOONGLINT_ARCHS})
  string(APPEND LOONGLINT_ENUM_ARCHS "LOONGLINT_ARCH(${arch})\n")
  string(APPEND LOONGLINT_ARCH_INCLUDES "#include \"loonglint/${arch}/${arch}Spec.hpp\"\n")
endforeach()

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/include/loonglint/ArchIncludes.inl.in
  ${CMAKE_CURRENT_BINARY_DIR}/include/loonglint/ArchIncludes.inl @ONLY)
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/include/loonglint/ArchConfig.def.in
  ${CMAKE_CURRENT_BINARY_DIR}/include/loonglint/ArchConfig.def @ONLY)

if(LLVM_INCLUDE_TESTS)
  add_subdirectory(unittests)
  add_subdirectory(test)
endif()
