#ifndef UTIL_H
#define UTIL_H

#include <cxxabi.h>
#include <memory>
#include <string>
#include <typeinfo>

#include "ast.h"

// Returns the demangled name of the class
inline std::string getASTClassName(ASTBase *node) {
  if (!node)
    return "null";

  if (dynamic_cast<ConstantExpr *>(node))
    return "ConstantExpr";
  if (dynamic_cast<IdentifierExpr *>(node))
    return "IdentifierExpr";
  if (dynamic_cast<CallExpr *>(node))
    return "CallExpr";
  if (dynamic_cast<ParenthesesExpression *>(node))
    return "ParenthesesExpression";
  if (dynamic_cast<BinaryExpression *>(node))
    return "BinaryExpression";
  if (dynamic_cast<ReturnStatement *>(node))
    return "ReturnStatement";
  if (dynamic_cast<AssignmentStatement *>(node))
    return "AssignmentStatement";
  if (dynamic_cast<FunctionDecl *>(node))
    return "FunctionDecl";

  const std::type_info &ti = typeid(*node);
  int status;
  std::unique_ptr<char, void (*)(void *)> demangled(
      abi::__cxa_demangle(ti.name(), nullptr, nullptr, &status), std::free);

  return (status == 0 && demangled) ? demangled.get() : ti.name();
};

template<typename T, typename U> 
bool isa(U a){
    if(dynamic_cast<T*>(a))
        return true;

    return false;
} 

template<typename T, typename U> 
T* dyncast(U a){
    if(T* casted = dynamic_cast<T*>(a))
        return casted;

    return nullptr;
} 

#endif
