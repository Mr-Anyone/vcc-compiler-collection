#ifndef SEMA_H
#define SEMA_H

#include <string>
#include "ast.h"
class Parser;

/// This entire idea is horrible for performance!
/// In a perfect world, this should be during parser
/// And not need to duplicate so much unnecessary code.
class Sema{
public:
    Sema();

    /// Perform a list of checks applies to function
    /// returns ture if passes, false otherwise
    bool checkFunction(FunctionDecl* decl);

private:
  static bool doesDeclareScope(ASTBase *base);
};

#endif
