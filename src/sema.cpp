#include "sema.h"
#include "util.h"
#include <iostream>
#include <memory>

Sema::Sema() {}

class ScopeDeclarationTree {
public:
  ScopeDeclarationTree() {}

  static bool doesDeclareScope(ASTBase *base) {
    if (isa<FunctionDecl>(base) ||
            isa<IfStatement>(base) ||
            isa<WhileStatement>(base))
        return true;

    return false;
  }

private:
};

bool Sema::checkFunction(FunctionDecl *function_decl) {
    // ensure we can always perform a valid name lookup
    for(ASTBase* statement: function_decl->getChildren()){

        // going inside the scope 
        if(ScopeDeclarationTree::doesDeclareScope(statement)){
            if(IfStatement* casted = dyncast<IfStatement>(statement)){

            }

            assert(false && "unreachable");

        }
    }
    
  return true;
}
