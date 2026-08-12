#include "core/sema.h"

#include "core/type.h"

using namespace vcc;

Sema::Sema(ContextHolder context) : m_context(context) {}

bool Sema::check(ASTBase* base)
{
    switch (base->getCode())
    {
        case code::FunctionDecl:
            return checkFunctionDecl(base->getAs<FunctionDecl>());
        case code::CallStatement:
            return checkCallStatement(base->getAs<CallStatement>());
        case code::CallExpr:
            return checkCallExpr(base->getAs<CallExpr>());
        case code::ReturnStatement:
            return checkReturnStatement(base->getAs<ReturnStatement>());
        default:
            return true;
    }

    return true;
}

bool Sema::checkCallStatement(CallStatement* statement)
{
    return check(statement->getCallExpression());
}

bool Sema::checkFunctionDecl(FunctionDecl* decl)
{
    for (ASTBase* stmt : decl->getStatements())
    {
        if (!check(stmt))
        {
            return false;
        }
    }

    return true;
}

bool Sema::checkCallExpr(CallExpr* expr)
{
    std::string name = expr->getFuncName();

    // First check if the function declaration exists
    Optional<FunctionDecl*> callee_decl =
        context()->getFunctionDeclTable().lookup(expr, name);
    if (callee_decl.isEmtpy())
    {
        return diag(expr, "function '" + name + "' cannot be found");
    }

    // Second check if the number of expression matches the number of parameters
    int input_parameter_count = expr->getExpressions().size();
    const std::vector<vcc::TypeInfo> callee_args =
        callee_decl->getArgList()->getArgs();
    if (input_parameter_count != callee_args.size())
    {
        return diag(expr, "parameter count mismatch");
    }

    // Check if all the types are the same
    bool has_type_error = false;
    for (auto [index, expr, type_info] :
         llvm::enumerate(expr->getExpressions(), callee_args))
    {
        if (!Type::isSame(expr->getType(context()), type_info.type))
        {
            diag(expr, "mismatch type between caller and callee");
            has_type_error = true;
        }
    }

    return !has_type_error;
}

bool Sema::checkReturnStatement(ReturnStatement* stmt)
{
    const FunctionDecl* decl = stmt->getFirstFunctionDecl();
    Type* decl_return_type   = decl->getReturnType();

    Expression* expr = stmt->getExpression();
    if (!expr && decl_return_type->isVoid())
    {
        return true;
    }

    if (!expr && !decl_return_type->isVoid())
    {
        diag(stmt, "function declaration and return statement disagree on return type");
        return diag(decl, "see here for function decl");
    }

    Type* infer_type = expr->getType(context());
    if (!Type::isSame(infer_type, decl_return_type))
    {
        return diag(expr, "return type and return expression gives different type");
    }

    return true;
}

/// ============ Start of sema helpers ============
Sema::DiagnosticResult Sema::diag(const ASTBase* base, std::string message)
{
    context()->getDiagnosticsDriver().diag(base, message);
    return DiagnosticResult{};
}

ContextHolder Sema::context()
{
    return m_context;
}
