#include "context.h"
#include "parser.h"
#include "driver.h"
#include <iostream>
#include <llvm/Support/Casting.h>

int main() {
    Parser parser = parseFile("testing.txt"); 
    ContextHolder holder = parser.getHolder();
    for(ASTBase* tree : parser.getSyntaxTree()){
        llvm::Function* func = 
            llvm::dyn_cast<llvm::Function>(tree->codegen(parser.getHolder()));
        func->dump();
    }

  return 0;
}
