set(
    FFI_PARSER
    main.cpp
)

add_llvm_executable(ffi_ir_parser ${FFI_PARSER})
# target_link_libraries(ffi_ir_parser comp ${llvm_libs})
