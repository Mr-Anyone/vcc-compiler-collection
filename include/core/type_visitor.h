#ifndef CORE_TYPE_VISITOR_H
#define CORE_TYPE_VISITOR_H

#include "ast_visitor.h"

namespace vcc
{

class TypeVisitor : public ASTVistor<Type*>
{
   protected:
    virtual Type* visitIdentifierExpr(IdentifierExpr* expression) override
    {
        VCC_UNREACHABLE("it is good that you have made it here for now");
        return nullptr;
    }

    virtual Type* visitCastExpression(CastExpression* expr) override
    {
        return expr->getType();
    }
};
};  // namespace vcc

#endif
