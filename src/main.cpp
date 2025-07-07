#include "parser.h"
#include <iostream>

int main(int argc, char* argv[]) {
  // FIXME: move this into its own function!
  ContextHolder context = std::make_shared<GlobalContext>();
  Parser parser("testing.txt", context);

  ASTBase *base = parser.buildSyntaxTree();
  ASTBase *base_two = parser.buildSyntaxTree();
  llvm::Function *function = llvm::cast<llvm::Function>(base->codegen(context));
  llvm::Function *function_two = llvm::cast<llvm::Function>(base_two->codegen(context));
  function->dump();
  function_two->dump();
  base->debugDump();
  base_two->debugDump();


  return 0;
}
