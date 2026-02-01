cmake_minimum_required(VERSION 3.8)
project(compiler)

# LLVM currently only supporting c++17
set(CMAKE_CXX_STANDARD 17)

# Enable testing and gtest related settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
enable_testing()

# LLVM related settings for CMAKE
set(LLVM_ENABLE_RTTI ON)
set(LLVM_TARGETS_TO_BUILD "X86")
add_subdirectory(external)

list(APPEND CMAKE_PREFIX_PATH "${CMAKE_BINARY_DIR}/external/llvm-project/llvm/lib/cmake/llvm")
find_package(LLVM REQUIRED CONFIG HINTS "${CMAKE_BINARY_DIR}/external/llvm-project/llvm/lib/cmake/llvm")

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

# add directory
add_subdirectory(src)
add_subdirectory(test)
