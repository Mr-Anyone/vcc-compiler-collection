#include "parser.h"
#include <iostream>

int main() {
  // FIXME: move this into its own function!
  ContextHolder context = std::make_shared<GlobalContext>();
  Parser parser("testing.txt", context);

  ASTBase *base = parser.buildSyntaxTree();
  ASTBase *func_two = parser.buildSyntaxTree();

  llvm::Function *function = llvm::cast<llvm::Function>(base->codegen(context));
  llvm::Function *fun_two_ir =
      llvm::cast<llvm::Function>(func_two->codegen(context));

  function->dump();
  fun_two_ir->dump();

  return 0;
}
