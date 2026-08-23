# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.20)

project(loonglint LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(LLVM 22.1 CONFIG REQUIRED)

if(NOT "LoongArch" IN_LIST LLVM_TARGETS_TO_BUILD)
  message(FATAL_ERROR "LLVM ${LLVM_PACKAGE_VERSION} was built without LoongArch support")
endif()

add_executable(loonglint
  src/main.cpp)

target_include_directories(loonglint SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND "${LLVM_DEFINITIONS}")
target_compile_options(loonglint PRIVATE ${LLVM_DEFINITIONS_LIST})

message(STATUS "Using LLVM ${LLVM_PACKAGE_VERSION} from ${LLVM_DIR}")

if(TARGET LLVM AND LLVM_LINK_LLVM_DYLIB)
  message(VERBOSE "LLVM library type: monolithic")
  target_link_libraries(loonglint PRIVATE LLVM)
else()
  message(VERBOSE "LLVM library type: individual")
  llvm_map_components_to_libnames(LOONGLINT_LLVM_LIBS
    Support
    Object
    MC
    MCDisassembler
    TargetParser
    LoongArchInfo
    LoongArchDesc
    LoongArchDisassembler)
  target_link_libraries(loonglint PRIVATE ${LOONGLINT_LLVM_LIBS})
endif()

include(CTest)
