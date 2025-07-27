set(
    LIB_FILES
    lex.cpp
    parser.cpp 
    context.cpp
    symbol_table.cpp
    ast.cpp
    sema.cpp
    driver.cpp
    # FIXME: maybe add this into a different standard library
    stream.cpp
    type.cpp
)

add_executable(main main.cpp)
target_link_libraries(main comp)

add_library(comp ${LIB_FILES})

target_include_directories(
    comp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

llvm_map_components_to_libnames(llvm_libs support core irreader x86codegen x86asmparser passes)
message(${llvm_libs})
target_link_libraries(comp ${llvm_libs})
