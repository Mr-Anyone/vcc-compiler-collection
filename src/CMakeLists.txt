set(
    LIB_FILES
    lex.cpp
    parser.cpp 
    context.cpp
    symbol_table.cpp
    ast.cpp
    sema.cpp
    # FIXME: maybe add this into a different standard library
    stream.cpp
)

add_executable(main main.cpp)
target_link_libraries(main comp)

add_library(comp ${LIB_FILES})

target_include_directories(
    comp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(comp LLVMCore LLVMSupport LLVMX86CodeGen LLVMX86AsmParser)
