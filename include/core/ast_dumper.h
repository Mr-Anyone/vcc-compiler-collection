#ifndef CORE_AST_DUMPER_H
#define CORE_AST_DUMPER_H

#include <ostream>
#include <sstream>
#include <string>

#include "core/ast_visitor.h"
#include "core/type.h"

namespace vcc
{

/// Formats a human-readable dump of AST nodes.
/// Each visit hook returns a std::string; dump/debugDump write to a stream.
class ASTDumper : public ASTVisitor<std::string>
{
   public:
    /// Format a single node's description (class name + node-specific details).
    std::string formatNode(ASTBase* node)
    {
        return visit(node);
    }

    /// Recursively dump the entire subtree rooted at `node` to `os`.
    void debugDump(ASTBase* node, std::ostream& os, int depth = 1)
    {
        printIndent(os, depth - 1);
        os << getASTClassName(node) << " " << formatNode(node) << "\n";

        for (ASTBase* child : node->getChildren())
        {
            printIndent(os, depth);
            debugDump(child, os, depth + 1);
        }
    }

   protected:
    // Statement hooks
    std::string visitFunctionDecl(FunctionDecl* decl) override
    {
        std::ostringstream ss;
        ss << "name: " << decl->getName() << " args: extern: " << decl->isExtern();
        for (auto it = decl->getArgBegin(), ie = decl->getArgsEnd(); it != ie; ++it)
        {
            ss << it->name << ", ";
        }
        return ss.str();
    }

    std::string visitDeclarationStatement(DeclarationStatement* stmt) override
    {
        return "name: " + stmt->getName();
    }

    // Expression hooks
    std::string visitConstantExpr(ConstantExpr* expr) override
    {
        return std::to_string(expr->getValue());
    }

    std::string visitIdentifierExpr(IdentifierExpr* expr) override
    {
        return "identifier: " + expr->getName();
    }

    std::string visitCallExpr(CallExpr* expr) override
    {
        return "name: " + expr->getFuncName();
    }

    std::string visitBinaryExpression(BinaryExpression* expr) override
    {
        switch (expr->getKind())
        {
            case BinaryExpression::Add:
                return "+";
            case BinaryExpression::Multiply:
                return "*";
            case BinaryExpression::Equal:
                return "equals";
            default:
                return "unknown";
        }
    }

    std::string visitMemberAccessExpression(MemberAccessExpression* expr) override
    {
        std::ostringstream ss;
        if (!expr->getParentExpression())
            ss << expr->getBaseName();

        ss << "." << expr->getMember() << " child: " << expr->getChildPosfixExpression()
           << " this: " << expr;
        return ss.str();
    }

    std::string visitArrayAccessExpression(ArrayAccessExpression* expr) override
    {
        std::ostringstream ss;
        ss << "[]" << " child*: " << expr->getChildPosfixExpression()
           << " this: " << expr;
        return ss.str();
    }

   private:
    static void printIndent(std::ostream& os, int depth)
    {
        for (int i = 0; i < depth * 2 - 1; ++i)
            os << " ";
    }
};

}  // namespace vcc

#endif
