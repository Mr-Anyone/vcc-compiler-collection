#ifndef CONTEXT_H
#define CONTEXT_H


// the owner of everything
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/IRBuilder.h>

#include <memory>

// Please use ContextHolder to access these variables
// useful for 
struct GlobalContext{
    GlobalContext(): 
        context(), builder(context), module("global module", context){

    }

    llvm::LLVMContext context;
    llvm::IRBuilder<> builder;
    llvm::Module module;
}; 

using ContextHolder = std::shared_ptr<GlobalContext>;
#endif
