set(
    LIB_FILES
    core/lex.cpp
    core/parser.cpp 
    core/context.cpp
    core/symbol_table.cpp
    core/ast.cpp
    core/sema.cpp
    core/driver.cpp
    core/type.cpp
    # FIXME: maybe add this into a different standard library
    core/stream.cpp
)

add_executable(vcc main.cpp)
target_link_libraries(vcc comp)

add_library(comp ${LIB_FILES})
target_include_directories(
    comp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/core/include
)

llvm_map_components_to_libnames(llvm_libs support core irreader x86codegen x86asmparser passes)
target_link_libraries(comp ${llvm_libs})

set(
    FFI_PARSER
    ffi/main.cpp
)

add_executable(ffi_ir_parser ${FFI_PARSER})
target_link_libraries(ffi_ir_parser comp ${llvm_libs})

# Installing Binaries
install(TARGETS ffi_ir_parser DESTINATION bin)
install(TARGETS vcc DESTINATION bin)
