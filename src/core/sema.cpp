#include "core/sema.h"
#include "core/util.h"
#include <iostream>
#include <memory>

using namespace vcc;

Sema::Sema() {}

bool Sema::checkFunction(FunctionDecl *function_decl) { return true; }
