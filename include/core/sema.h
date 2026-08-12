#ifndef CORE_SEMA_H
#define CORE_SEMA_H

#include "ast.h"
#include "context.h"

namespace vcc
{
class Parser;

class Sema
{
   public:
    Sema(ContextHolder context);
    bool check(ASTBase* node);

   private:
    /// Semantics analysis on functions
    bool checkFunctionDecl(FunctionDecl* decl);

    /// Checks for statement
    bool checkCallStatement(CallStatement* stmt);
    bool checkReturnStatement(ReturnStatement* stmt);

    bool checkCallExpr(CallExpr* expr);

    /// Diagnostics helper
    struct DiagnosticResult
    {
        // The sema class return a bool value
        inline operator bool()
        {
            return false;
        }
    };
    DiagnosticResult diag(const ASTBase* base, std::string message);

    ContextHolder context();
    ContextHolder m_context;
};

};  // namespace vcc
#endif
