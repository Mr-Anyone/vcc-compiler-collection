//===- ast_visitor.h - AST visitor base class ----===//
//
// Part of the VCC Compiler Collection.
//
//===----===//

#ifndef CORE_AST_VISITOR_H
#define CORE_AST_VISITOR_H

#include <cassert>
#include <type_traits>

#include "core/ast.h"
#include "core/util.h"

namespace vcc
{
template <typename ReturnT>
class ASTVisitor
{
    static_assert(std::is_void_v<ReturnT> || std::is_default_constructible_v<ReturnT>,
                  "ReturnT must be void or default-constructible");

   public:
    virtual ~ASTVisitor() = default;

    /// Single public entry point — dispatches to the correct visit hook
    /// based on whether `node` is a Statement or Expression.
    ReturnT visit(ASTBase* node)
    {
        if (auto* stmt = dyncast<Statement>(node))
            return visitStatement(stmt);

        if (auto* expr = dyncast<Expression>(node))
            return visitExpression(expr);

        assert(false && "node is neither Statement nor Expression");
        return ReturnT();
    }

   protected:
    // Statement hooks
    virtual ReturnT visitCallStatement(CallStatement*)
    {
        return ReturnT();
    }
    virtual ReturnT visitFunctionArgLists(FunctionArgLists*)
    {
        return ReturnT();
    }
    virtual ReturnT visitFunctionDecl(FunctionDecl*)
    {
        return ReturnT();
    }
    virtual ReturnT visitAssignmentStatement(AssignmentStatement*)
    {
        return ReturnT();
    }
    virtual ReturnT visitReturnStatement(ReturnStatement*)
    {
        return ReturnT();
    }
    virtual ReturnT visitDeclarationStatement(DeclarationStatement*)
    {
        return ReturnT();
    }
    virtual ReturnT visitIfStatement(IfStatement*)
    {
        return ReturnT();
    }
    virtual ReturnT visitWhileStatement(WhileStatement*)
    {
        return ReturnT();
    }

    // Expression hooks
    virtual ReturnT visitConstantExpr(ConstantExpr*)
    {
        return ReturnT();
    }
    virtual ReturnT visitCallExpr(CallExpr*)
    {
        return ReturnT();
    }
    virtual ReturnT visitBinaryExpression(BinaryExpression*)
    {
        return ReturnT();
    }
    virtual ReturnT visitCastExpression(CastExpression*)
    {
        return ReturnT();
    }
    virtual ReturnT visitIdentifierExpr(IdentifierExpr*)
    {
        return ReturnT();
    }
    virtual ReturnT visitMemberAccessExpression(MemberAccessExpression*)
    {
        return ReturnT();
    }
    virtual ReturnT visitArrayAccessExpression(ArrayAccessExpression*)
    {
        return ReturnT();
    }
    virtual ReturnT visitDeRefExpression(DeRefExpression*)
    {
        return ReturnT();
    }
    virtual ReturnT visitRefExpression(RefExpression*)
    {
        return ReturnT();
    }
    virtual ReturnT visitStringLiteral(StringLiteral*)
    {
        return ReturnT();
    }

   private:
    inline ReturnT visitStatement(Statement* stmt)
    {
        switch (stmt->getCode())
        {
            case code::CallStatement:
                return visitCallStatement(dyncast<CallStatement>(stmt));
            case code::FunctionArgLists:
                return visitFunctionArgLists(dyncast<FunctionArgLists>(stmt));
            case code::FunctionDecl:
                return visitFunctionDecl(dyncast<FunctionDecl>(stmt));
            case code::AssignmentStatement:
                return visitAssignmentStatement(dyncast<AssignmentStatement>(stmt));
            case code::ReturnStatement:
                return visitReturnStatement(dyncast<ReturnStatement>(stmt));
            case code::DeclarationStatement:
                return visitDeclarationStatement(dyncast<DeclarationStatement>(stmt));
            case code::IfStatement:
                return visitIfStatement(dyncast<IfStatement>(stmt));
            case code::WhileStatement:
                return visitWhileStatement(dyncast<WhileStatement>(stmt));
            default:
                assert(false && "unhandled statement kind in ASTVisitor::visitStatement");
                return ReturnT();
        }
    }

    inline ReturnT visitExpression(Expression* expr)
    {
        switch (expr->getCode())
        {
            case code::ConstantExpr:
                return visitConstantExpr(dyncast<ConstantExpr>(expr));
            case code::CallExpr:
                return visitCallExpr(dyncast<CallExpr>(expr));
            case code::BinaryExpression:
                return visitBinaryExpression(dyncast<BinaryExpression>(expr));
            case code::CastExpression:
                return visitCastExpression(dyncast<CastExpression>(expr));
            case code::IdentifierExpr:
                return visitIdentifierExpr(dyncast<IdentifierExpr>(expr));
            case code::MemberAccessExpression:
                return visitMemberAccessExpression(dyncast<MemberAccessExpression>(expr));
            case code::ArrayAccessExpression:
                return visitArrayAccessExpression(dyncast<ArrayAccessExpression>(expr));
            case code::DeRefExpression:
                return visitDeRefExpression(dyncast<DeRefExpression>(expr));
            case code::RefExpression:
                return visitRefExpression(dyncast<RefExpression>(expr));
            case code::StringLiteral:
                return visitStringLiteral(dyncast<StringLiteral>(expr));
            default:
                assert(false &&
                       "unhandled expression kind in ASTVisitor::visitExpression");
                return ReturnT();
        }
    }
};

}  // namespace vcc

#endif
