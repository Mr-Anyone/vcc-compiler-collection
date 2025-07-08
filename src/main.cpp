#include "parser.h"
#include <iostream>

int main(int argc, char* argv[]) {
  // FIXME: move this into its own function!
  ContextHolder context = std::make_shared<GlobalContext>();
  Parser parser("testing.txt", context);

  ASTBase *base = parser.buildSyntaxTree();
  ASTBase *base_two = parser.buildSyntaxTree();
  ASTBase *base_three = parser.buildSyntaxTree();


  llvm::Function *function = llvm::cast<llvm::Function>(base->codegen(context));
  llvm::Function *function_two = llvm::cast<llvm::Function>(base_two->codegen(context));
  llvm::Function *function_three = llvm::cast<llvm::Function>(base_three->codegen(context));
  function->dump();
  function_two->dump();
  function_three->dump();


  return 0;
}
