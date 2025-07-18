#include "context.h"
#include "parser.h"
#include "driver.h"
#include "util.h"

#include <iostream>
#include <llvm/Support/Casting.h>

int main() {
    Parser parser = parseFile("testing.txt"); 
    ContextHolder holder = parser.getHolder();
    Sema sema; 
    // lth
    for(ASTBase* tree : parser.getSyntaxTree()){
        bool is_good = sema.checkFunction(dyncast<FunctionDecl>(tree));
        assert(is_good);

        llvm::Function* func = 
            llvm::dyn_cast<llvm::Function>(tree->codegen(parser.getHolder()));
    }
    holder->module.dump();

  return 0;
}
