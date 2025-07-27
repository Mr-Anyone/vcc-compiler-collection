#ifndef CONTEXT_H
#define CONTEXT_H

// the owner of everything
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>

#include "symbol_table.h"
#include <memory>

// Please use ContextHolder to access these variables
// useful for
struct GlobalContext {
  GlobalContext();

  llvm::LLVMContext context;
  llvm::IRBuilder<> builder;
  llvm::Module module;

  // symbol table
  SymbolTable symbol_table;
};

using ContextHolder = std::shared_ptr<GlobalContext>;
#endif
