#include "sema.h"
#include "util.h"
#include <iostream>
#include <memory>

Sema::Sema() {}

bool Sema::checkFunction(FunctionDecl *function_decl) {
  return true;
}
