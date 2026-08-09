#ifndef AST_H
#define AST_H

#include <string>
#include <vector>

#include "core/lex.h"
#include "core/rtti.h"

namespace vcc
{
class Expression;
class Statement;
class FunctionDecl;
class Scope;

namespace code
{
enum TreeCode
{
    // Statements
    FunctionArgLists,
    CallStatement,
    FunctionDecl,
    AssignmentStatement,
    ReturnStatement,
    DeclarationStatement,
    IfStatement,
    WhileStatement,

    // Expression
    ConstantExpr,
    CallExpr,
    BinaryExpression,
    CastExpression,
    IdentifierExpr,
    MemberAccessExpression,
    ArrayAccessExpression,
    DeRefExpression,
    RefExpression,
    StringLiteral
};
};

class ASTBase
{
   public:
    virtual ~ASTBase() = default;

    ASTBase(code::TreeCode code, const std::vector<Expression*> childrens, FilePos pos,
            Scope* scope);
    ASTBase(code::TreeCode code, const std::vector<Statement*> childrens, FilePos pos,
            Scope* scope);

    // nullptr on failure
    const FunctionDecl* getFirstFunctionDecl() const;

    const ASTBase* getParent() const;
    const std::set<ASTBase*>& getChildren() const;
    const FilePos& getPos() const;

    void debugDump(int depth = 1);

    inline code::TreeCode getCode() const
    {
        return m_code;
    }

    bool doesDefineScope() const;

    /// A nice helper to improve upon style. Instead of doing, dyncast<...>(variable),
    /// we can do this->getAs<someType>();
    template <typename T>
    T* getAs()
    {
        return dyncast<T>(this);
    }

    Scope* getScope();

   protected:
    void setParent(ASTBase* parent);
    void addChildren(ASTBase* children);
    void removeChildren(ASTBase* children);

   private:
    code::TreeCode m_code;
    FilePos m_locus;
    ASTBase* m_parent;
    Scope* m_scope;
    std::string m_name;
    std::set<ASTBase*> m_childrens;
};
};  // namespace vcc

#endif
