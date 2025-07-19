#include "context.h"
#include "parser.h"
#include "driver.h"
#include "util.h"

#include <iostream>
#include <llvm/Support/Casting.h>
#include <llvm/Support/CommandLine.h>

llvm::cl::opt<bool> print_ast ("print-ast", llvm::cl::desc("Whether to print syntax tree"));
llvm::cl::opt<bool> print_llvm ("print-llvm", llvm::cl::desc("Whether to print llvm"));
llvm::cl::opt<std::string> input_filename (llvm::cl::Positional, llvm::cl::Required, 
        llvm::cl::desc("<input filename>"));

int main(int argc, char*argv []) {
    llvm::cl::ParseCommandLineOptions(argc, argv);

    Parser parser = parseFile(input_filename.c_str()); 
    ContextHolder holder = parser.getHolder();
    Sema sema; 

    for(ASTBase* tree : parser.getSyntaxTree()){
        if(print_ast)
            tree->debugDump();

        llvm::Function* func = 
            llvm::dyn_cast<llvm::Function>(tree->codegen(parser.getHolder()));
    }

    if(print_llvm)
        holder->module.dump();

  return 0;
}
