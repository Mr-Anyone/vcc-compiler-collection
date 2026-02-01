add_subdirectory(core)
#add_subdirectory(ffi)

add_llvm_executable(vcc main.cpp)
target_link_libraries(vcc comp)
