#include "symbol_table.h"
#include "ast.h"

SymbolTable::SymbolTable() : m_local_table(), m_function_table() {}

void SymbolTable::addFunction(FunctionDecl *function_decl) {
  const std::string &function_name = function_decl->getName();
  llvm::Function *function = function_decl->getLLVMFunction();
  assert(!m_function_table.contains(function_name));

  m_function_table[function_name] = function;
}

llvm::Function *SymbolTable::lookupFunction(const std::string &name) {
  assert(m_function_table.contains(name));
  return m_function_table[name];
}

void SymbolTable::addLocalVariable(FunctionDecl *function, std::string name,
                                   llvm::Value *value) {
  // reserved for implementation
  assert(name.find('$') != name.size() && "cannot contain $");

  std::string lookup_name = makeLocalVariableLookupName(function, name);
  m_local_table[lookup_name] = value;
}

llvm::Value *SymbolTable::lookupLocalVariable(FunctionDecl *function,
                                              std::string name) {
  std::string lookup_name = makeLocalVariableLookupName(function, name);
  assert(m_local_table.contains(lookup_name));
  return m_local_table[lookup_name];
}

std::string SymbolTable::makeLocalVariableLookupName(FunctionDecl *function,
                                                     std::string name) {
  return function->getName() + "$" + name;
}
