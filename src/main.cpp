#include "parser.h"

int main() {
  // FIXME: move this into its own function!
  ContextHolder context = std::make_shared<GlobalContext>();
  Parser parser("testing.txt", context);
  ASTBase *base = parser.buildSyntaxTree();
  base->codegen(context)->dump();

  return 0;
}
