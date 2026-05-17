#ifndef CORE_AST_H
#define CORE_AST_H

#include <llvm/IR/Attributes.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>

#include <string>
#include <vector>

#include "core/context.h"
#include "core/lex.h"
#include "core/rtti.h"

namespace llvm
{
class FunctionType;
}

namespace vcc
{
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

struct TypeInfo;
class BuiltinType;

class FunctionDecl;
class Expression;
class Statement;

class ASTBase
{
   public:
    virtual ~ASTBase() = default;

    ASTBase(code::TreeCode code, const std::vector<Expression*> childrens, FilePos pos);
    ASTBase(code::TreeCode code, const std::vector<Statement*> childrens, FilePos pos);

    // nullptr on failure
    const FunctionDecl* getFirstFunctionDecl() const;

    // gets the first ASTBase that defines a scope
    // nullptr on failure, get the first ASTBase that represents a scope
    const ASTBase* getScopeDeclLoc() const;

    const ASTBase* getParent() const;
    const std::set<ASTBase*>& getChildren() const;
    const FilePos& getPos() const;

    void debugDump(int depth = 1);

    inline code::TreeCode getCode() const
    {
        return m_code;
    }

    static bool doesDefineScope(const ASTBase* at);
    static bool doesDefineScope(code::TreeCode code);

    /// A nice helper to improve upon style. Instead of doing, dyncast<...>(variable),
    /// we can do this->getAs<someType>();
    template<typename T>
    T* getAs()
    {
        return dyncast<T>(this);
    }

   protected:
    void setParent(ASTBase* parent);
    void addChildren(ASTBase* children);
    void removeChildren(ASTBase* children);

   private:
    code::TreeCode m_code;
    FilePos m_locus;
    ASTBase* m_parent;
    std::string m_name;
    std::set<ASTBase*> m_childrens;
};

//============================== Statements ==============================
class Statement : public ASTBase
{
   public:
    Statement(code::TreeCode code, const std::vector<ASTBase*> childrens, FilePos locus);

   private:
};

class CallStatement : public Statement
{
   public:
    CallStatement(Expression* call_expression, FilePos locus);

    Expression* getCallExpression() const;

   private:
    Expression* m_call_expr;
};

class FunctionArgLists : public Statement
{
   public:
    using ArgsIter = std::vector<TypeInfo>::const_iterator;

    FunctionArgLists(std::vector<TypeInfo>&& args, FilePos locus);

    ArgsIter begin() const;
    ArgsIter end() const;
    const std::vector<TypeInfo>& getArgs() const;

   private:
    std::vector<TypeInfo> m_args;
};

// FIXME: we should separate FunctionBody with FunctionDecl
class FunctionDecl : public Statement
{
   public:
    /// if `is_extern` is true, codegen only generate a declaration and assume
    /// to have no body
    FunctionDecl(std::vector<Statement*>& expression, FunctionArgLists* arg_list,
                 std::string&& name, Type* return_type, bool is_extern, FilePos locus);

    const std::string& getName() const;
    Type* getReturnType() const;
    llvm::FunctionType* getFunctionType(ContextHolder holder) const;

    const FunctionArgLists::ArgsIter getArgBegin() const;
    const FunctionArgLists::ArgsIter getArgsEnd() const;

    bool isExtern() const;
    const std::vector<Statement*>& getStatements() const;
    FunctionArgLists* getArgList() const;

   private:
    bool m_is_extern;  // is external or not?

    Type* m_return_type;
    std::vector<Statement*> m_statements;
    FunctionArgLists* m_arg_list;
    std::string m_name;
};

class AssignmentStatement : public Statement
{
   public:
    AssignmentStatement(Expression* ref_expression, Expression* expression,
                        FilePos locus);

    const std::string& getName();

    Expression* getRefExpression() const;
    Expression* getExpression() const;

   private:
    Expression* m_ref_expr;  // The right hand side of the equation
    Expression* m_expression;
};

class ReturnStatement : public Statement
{
   public:
    // returning an identifier
    ReturnStatement(Expression* expression, FilePos locus);

    Expression* getExpression() const;

   private:
    // this gives some sort of value
    Expression* m_expression;
};

class DeclarationStatement : public Statement
{
   public:
    // if expression is nullptr, it means that we just allocate space
    // and don't assign it to the thing
    DeclarationStatement(const std::string& name, Expression* expression, Type* type,
                         FilePos locus);

    Expression* getExpression();
    Type* getType();
    const std::string& getName();

   private:
    std::string m_name;
    Expression* m_expression;
    Type* m_type;
};

class IfStatement : public Statement
{
   public:
    IfStatement(Expression* cond, std::vector<Statement*>&& expressions, FilePos locus);

    std::vector<DeclarationStatement*> getDeclarationStatements() const;
    Expression* getCondition() const;
    const std::vector<Statement*>& getStatements() const;

   private:
    // m_cond is a expression which may or may not be i1.
    // this is a terrible name
    Expression* m_cond;
    std::vector<Statement*> m_statements;
};

class WhileStatement : public Statement
{
   public:
    WhileStatement(Expression* cond, std::vector<Statement*>&& expressions,
                   FilePos locus);

    std::vector<DeclarationStatement*> getDeclarationStatements() const;
    Expression* getCondition() const;
    const std::vector<Statement*>& getStatements() const;

   private:
    Expression* m_cond;
    std::vector<Statement*> m_statements;
};

//============================== Expressions ==============================
// These are expressions that yields some sort of value
class Expression : public ASTBase
{
   public:
    Expression(code::TreeCode code, const std::vector<Expression*> childrens,
               FilePos locus);
    virtual Type* getType(ContextHolder holder) = 0;
};

/// Basically like an L value in c++,
/// This is something that returns an value
class LocatorExpression : public Expression
{
   public:
    LocatorExpression(code::TreeCode code, const std::vector<Expression*>& childrens,
                      FilePos locus);

   protected:
    friend class MemberAccessExpression;
    friend class ArrayAccessExpression;
};

class ConstantExpr : public Expression
{
   public:
    explicit ConstantExpr(int value, FilePos locus);
    virtual Type* getType(ContextHolder holder) override;

    int getValue();

   private:
    int m_value;
};

class CallExpr : public Expression
{
   public:
    CallExpr(const std::string& name, const std::vector<Expression*>& expressions,
             FilePos locus);

    virtual Type* getType(ContextHolder holder) override;

    const std::string& getFuncName() const;
    const std::vector<Expression*>& getExpressions() const;

   private:
    std::string m_func_name;
    std::vector<Expression*> m_expressions;
};

class BinaryExpression : public Expression
{
   public:
    enum BinaryExpressionType
    {
        Add,
        Subtract,
        Multiply,
        Equal,
        NEquals,
        GE,
        GT,
        LE,
        LT,
        Divide,
    };
    static BinaryExpressionType getFromLexType(lex::Token lex_type);
    virtual Type* getType(ContextHolder holder) override;

    BinaryExpression(Expression* lhs, BinaryExpressionType type, FilePos locus);

    void setRHS(Expression* rhs);

    Expression* getLHS() const;
    Expression* getRHS() const;
    BinaryExpressionType getKind() const;

   private:
    Expression* m_lhs;
    Expression* m_rhs;
    BinaryExpressionType m_kind;
};

// For the following expression
//
// float a;
// cast<int>(a);
class CastExpression : public Expression
{
   public:
    CastExpression(Expression* cast_expression, Type* casted_to, FilePos loc);

    virtual Type* getType(ContextHolder holder) override;

    Expression* getCastedExpression() const;
    Type* getCastTo() const;

   private:
    Expression* m_to_be_casted_expression;
    Type* m_cast_to;
};

/// === START OF LocatorExpression ===
class IdentifierExpr : public LocatorExpression
{
   public:
    /// Create an identifier expression
    ///
    /// name - the name of the identifier/variable
    /// compute_ref - true if codegen returns an address, otherwise returns the
    /// value to the identifier
    IdentifierExpr(const std::string& name, FilePos locus);

    virtual Type* getType(ContextHolder holder) override;

    const std::string& getName() const;

   private:
    std::string m_name;
};

// FIXME: maybe we should do type deduction here instead!
// The parser parse enough type so that this won't be a problem
class MemberAccessExpression : public LocatorExpression
{
   public:
    MemberAccessExpression(const std::string& name, const std::string& member,
                           FilePos locus);

    // from nested postfix-expression
    MemberAccessExpression(LocatorExpression* parent, const std::string& member,
                           FilePos locus);

    virtual Type* getType(ContextHolder holder) override;

    Type* getGEPType(ContextHolder holder);
    Type* getGEPChildType(ContextHolder holder);

    void setChildPosfixExpression(LocatorExpression* child);

    LocatorExpression* getParentExpression() const;
    const std::string& getBaseName() const;
    const std::string& getMember() const;
    LocatorExpression* getChildPosfixExpression() const;

   private:
    // either we have a m_base_name for symbol lookup or we must have a parent
    // expression
    LocatorExpression *m_parent = nullptr, *m_child_posfix_expression = nullptr;
    std::string m_base_name;  // only used when m_parent == nullptr
    std::string m_member;     // the member we are accessing
};

// FIXME: maybe we should do type deduction here instead!
class ArrayAccessExpression : public LocatorExpression
{
   public:
    ArrayAccessExpression(const std::string& name, Expression* expression, FilePos locus);
    ArrayAccessExpression(LocatorExpression* parent, Expression* expression,
                          FilePos locus);

    Type* getGEPType(ContextHolder holder);
    Type* getGEPChildType(ContextHolder holder);
    virtual Type* getType(ContextHolder holder) override;

    void setChildPosfixExpression(LocatorExpression* child);

    Expression* getIndexExpression() const;
    const std::string& getBaseName() const;
    LocatorExpression* getParentExpression() const;
    LocatorExpression* getChildPosfixExpression() const;

   private:
    Expression* m_index_expression;  // the index number
    // either we have a m_base_name for symbol lookup or we must have a parent
    // expression
    std::string m_base_name;  //
    LocatorExpression *m_parent_expression = nullptr,
                      *m_child_posfix_expression;  // the member we are accessing
};

// This is a weird expression
// because this both define a reference and a value
//
// int a = deref<b>; # `getVal` returns the value of the pointer
// deref<a> = 10 #  `` return the address of the pointee
class DeRefExpression : public LocatorExpression
{
   public:
    DeRefExpression(Expression* ref_get, FilePos locus);

    virtual Type* getType(ContextHolder holder) override;

    Type* getInnerType(ContextHolder holder);

    void setPosfixChildExpression(LocatorExpression* expression);

    Expression* getRef() const;
    LocatorExpression* getPosfixChild() const;

   private:
    Expression* m_ref;
    LocatorExpression* m_posfix_child = nullptr;  // it is possible for this to be the
                                                  // parent of a posfix expression.
                                                  // deref<a>.a.c[10] = 10; # for example
};

// This is similar to DeRefExpression, but does the opposite thing
//
// int c = 10;
// ptr int a = ref<c>; # this makes well form
class RefExpression : public LocatorExpression
{
   public:
    RefExpression(Expression* inner, FilePos locus);

    virtual Type* getType(ContextHolder holder) override;

    Expression* getInnerExpression() const;

   private:
    Expression* m_inner_expression;
};

// For the following:
//
// ptr char some_string = "this is my first string";
class StringLiteral : public Expression
{
   public:
    StringLiteral(std::string string, FilePos locus);

    virtual Type* getType(ContextHolder holder) override;

    const std::string& getString() const;

   private:
    std::string m_string_literal;
};
}  // namespace vcc

#endif
