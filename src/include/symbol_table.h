#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <iostream>
#include <llvm/IR/Value.h>

class FunctionDecl;

// Basically just like a hash table
class SymbolTable {
public:
  SymbolTable();

  // FIXME: do we really need two arguments?
  void addFunction(FunctionDecl *function_decl);
  llvm::Function *lookupFunction(const std::string &name);

  //
  // Adding and looking up local variable
  void addLocalVariable(FunctionDecl *function, std::string name,
                        llvm::Value *value);
  // FIXME: it may be better to just return a struct that contains a bit
  // more type information
  llvm::Value *lookupLocalVariable(FunctionDecl *function, std::string name);
  bool containsLocalVariable(FunctionDecl* function, const std::string& name);

private:
  inline std::string makeLocalVariableLookupName(FunctionDecl *function,
                                                 std::string name);

  std::unordered_map<std::string, llvm::Value *> m_local_table;
  std::unordered_map<std::string, llvm::Function *> m_function_table;
};

#endif
