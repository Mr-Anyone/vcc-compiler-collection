cmake_minimum_required(VERSION 3.8)
project(compiler)

# LLVM currently only supporting c++17
set(CMAKE_CXX_STANDARD 17)

# Enable testing 
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
enable_testing()

# We only support X86 build type for now 
set(LLVM_TARGETS_TO_BUILD X86)
add_subdirectory(external)

add_subdirectory(src)
add_subdirectory(test)
