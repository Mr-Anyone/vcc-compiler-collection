#include "parser.h"
#include <iostream>

int main() {
  // FIXME: move this into its own function!
  ContextHolder context = std::make_shared<GlobalContext>();
  Parser parser("testing.txt", context);
  ASTBase *base = parser.buildSyntaxTree();

  llvm::Function *function = llvm::cast<llvm::Function>(base->codegen(context));
  function->dump();

  return 0;
}
