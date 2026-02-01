add_library(comp
  lex.cpp
  parser.cpp 
  context.cpp
  symbol_table.cpp
  ast.cpp
  sema.cpp
  driver.cpp
  type.cpp

  # FIXME: maybe add this into a different standard library
  stream.cpp
)

llvm_map_components_to_libnames(llvm_libs support core irreader x86codegen x86asmparser passes)
target_link_libraries(comp ${llvm_libs})

target_include_directories(
  comp
  PUBLIC
  ${PROJECT_SOURCE_DIR}/include
)
