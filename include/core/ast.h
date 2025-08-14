#ifndef CORE_AST_H
#define CORE_AST_H

#include <llvm/IR/Attributes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <string>
#include <vector>

#include "core/context.h"
#include "core/lex.h"

// defined in type.h
namespace vcc {
namespace code {
enum TreeCode {
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

class ASTBase {
public:
  virtual void dump();

  ASTBase(const std::vector<Expression *> childrens, FilePos pos);
  ASTBase(const std::vector<Statement *> childrens, FilePos pos);

  // nullptr on failure
  const FunctionDecl *getFirstFunctionDecl() const;

  // gets the first ASTBase that defines a scope
  // nullptr on failure, get the first ASTBase that represents a scope
  const ASTBase *getScopeDeclLoc() const;

  const ASTBase *getParent() const;
  const std::set<ASTBase *> &getChildren() const;
  const FilePos &getPos() const;

  void debugDump(int depth = 1);

  virtual code::TreeCode getCode() const = 0;

  static bool doesDefineScope(const ASTBase *at);
  static bool doesDefineScope(code::TreeCode code);

protected:
  void setParent(ASTBase *parent);
  void addChildren(ASTBase *children);
  void removeChildren(ASTBase *children);

private:
  FilePos m_locus;
  ASTBase *m_parent;
  std::string m_name;
  std::set<ASTBase *> m_childrens;
};

//============================== Statements ==============================
class Statement : public ASTBase {
public:
  Statement(const std::vector<ASTBase *> childrens, FilePos locus);
  virtual void codegen(ContextHolder holder) = 0;

private:
};

class CallStatement : public Statement {
public:
  CallStatement(Expression *call_expression, FilePos locus);

  virtual void codegen(ContextHolder holder) override;
  virtual code::TreeCode getCode() const override;

private:
  Expression *m_call_expr;
};

class FunctionArgLists : public Statement {
public:
  using ArgsIter = std::vector<TypeInfo>::const_iterator;

  FunctionArgLists(std::vector<TypeInfo> &&args, FilePos locus);

  // the first few alloc, and load instruction
  virtual void codegen(ContextHolder holder) override;
  virtual code::TreeCode getCode() const override;

  ArgsIter begin() const;
  ArgsIter end() const;

private:
  std::vector<TypeInfo> m_args;
};

// FIXME: we should separate FunctionBody with FunctionDecl
class FunctionDecl : public Statement {
public:
  /// if `is_extern` is true, codegen only generate a declaration and assume to
  /// have no body
  FunctionDecl(std::vector<Statement *> &expression, FunctionArgLists *arg_list,
               std::string &&name, Type *return_type, bool is_extern,
               FilePos locus);

  virtual void codegen(ContextHolder holder) override;
  virtual code::TreeCode getCode() const override;

  void dump() override;

  const std::string &getName() const;
  llvm::Function *getLLVMFunction() const;
  Type *getReturnType() const;
  llvm::FunctionType *getFunctionType(ContextHolder holder) const;

  const FunctionArgLists::ArgsIter getArgBegin() const;
  const FunctionArgLists::ArgsIter getArgsEnd() const;

private:
  void buildExternalDecl(ContextHolder holder);
  void emitAllocs(ContextHolder holder);

  bool m_is_extern; // is external or not?

  Type *m_return_type;
  std::vector<Statement *> m_statements;
  FunctionArgLists *m_arg_list;
  std::string m_name;

  // nullptr before codegen
  llvm::Function *m_function = nullptr;
};

class AssignmentStatement : public Statement {
public:
  AssignmentStatement(Expression *ref_expression, Expression *expression,
                      FilePos locus);

  virtual void codegen(ContextHolder holder) override;
  virtual void dump() override;
  virtual code::TreeCode getCode() const override;
  const std::string &getName();

private:
  Expression *m_ref_expr; // The right hand side of the equation
  Expression *m_expression;
};

class ReturnStatement : public Statement {
public:
  // returning an identifier
  ReturnStatement(Expression *expression, FilePos locus);

  virtual void codegen(ContextHolder holder) override;
  virtual code::TreeCode getCode() const override;

private:
  // this gives some sort of value
  Expression *m_expression;
};

class DeclarationStatement : public Statement {
public:
  // if expression is nullptr, it means that we just allocate space
  // and don't assign it to the thing
  DeclarationStatement(const std::string &name, Expression *expression,
                       Type *type, FilePos locus);

  virtual void dump() override;
  virtual void codegen(ContextHolder holder) override;
  virtual code::TreeCode getCode() const override;
  Expression* getExpression();
  Type* getType();
  const std::string& getName();
private:

  std::string m_name;
  Expression *m_expression;
  Type *m_type;
};

class IfStatement : public Statement {
public:
  IfStatement(Expression *cond, std::vector<Statement *> &&expressions,
              FilePos locus);
  virtual void dump() override;
  virtual void codegen(ContextHolder holder) override;
  virtual code::TreeCode getCode() const override;

  std::vector<DeclarationStatement*> getDeclarationStatements() const;
private:
  // m_cond is a expression which may or may not be i1.
  // this is a terrible name
  Expression *m_cond;
  std::vector<Statement *> m_statements;
};

class WhileStatement : public Statement {
public:
  WhileStatement(Expression *cond, std::vector<Statement *> &&expressions,
                 FilePos locus);
  virtual void codegen(ContextHolder holder) override;
  virtual void dump() override;
  virtual code::TreeCode getCode() const override;

  std::vector<DeclarationStatement*> getDeclarationStatements() const;

private:
  Expression *m_cond;
  std::vector<Statement *> m_statements;
};

//============================== Expressions ==============================
// These are expressions that yields some sort of value
class Expression : public ASTBase {
public:
  Expression(const std::vector<Expression *> childrens, FilePos locus);
  virtual Type *getType(ContextHolder holder) = 0;
  virtual llvm::Value *getVal(ContextHolder holder) = 0;
};

/// Basically like an L value in c++,
/// This is something that returns an value
class LocatorExpression : public Expression {
public:
  LocatorExpression(const std::vector<Expression *> &childrens, FilePos locus);

  /// recursively traverse the tree to get the reference to
  /// the current type
  virtual llvm::Value *getRef(ContextHolder holder);

protected:
  friend class MemberAccessExpression;
  friend class ArrayAccessExpression;
};

class ConstantExpr : public Expression {
public:
  explicit ConstantExpr(int value, FilePos locus);
  virtual void dump() override;
  virtual llvm::Value *getVal(ContextHolder holder) override;
  virtual Type *getType(ContextHolder holder) override;
  virtual code::TreeCode getCode() const override;

  int getValue();

private:
  int m_value;
};

class CallExpr : public Expression {
public:
  CallExpr(const std::string &name,
           const std::vector<Expression *> &expressions, FilePos locus);
  llvm::Value *getVal(ContextHolder holder) override;
  void dump() override;

  virtual Type *getType(ContextHolder holder) override;
  virtual code::TreeCode getCode() const override;

private:
  std::string m_func_name;
  std::vector<Expression *> m_expressions;
};

class BinaryExpression : public Expression {
public:
  enum BinaryExpressionType {
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
  virtual Type *getType(ContextHolder holder) override;
  virtual code::TreeCode getCode() const override;

public:
  BinaryExpression(Expression *lhs, BinaryExpressionType type, FilePos locus);

  virtual void dump() override;
  virtual llvm::Value *getVal(ContextHolder holder) override;

  void setRHS(Expression *rhs);

private:
  llvm::Value *handleInteger(ContextHolder holder, llvm::Value *lhs,
                             llvm::Value *rhs);

  Expression *m_lhs;
  Expression *m_rhs;
  BinaryExpressionType m_kind;
};

// For the following expression
//
// float a;
// cast<int>(a);
class CastExpression : public Expression {
public:
  CastExpression(Expression *cast_expression, Type *casted_to, FilePos loc);

  virtual llvm::Value *getVal(ContextHolder holder) override;
  virtual Type *getType(ContextHolder holder) override;
  virtual code::TreeCode getCode() const override;

private:
  llvm::Value *builtinCast(BuiltinType *from, BuiltinType *to,
                           ContextHolder holder);

  void emitErrorAndExit(ContextHolder holder);
  Expression *m_to_be_casted_expression;
  Type *m_cast_to;
};

/// === START OF LocatorExpression ===
class IdentifierExpr : public LocatorExpression {
public:
  /// Create an identifier expression
  ///
  /// name - the name of the identifier/variable
  /// compute_ref - true if codegen returns an address, otherwise returns the
  /// value to the identifier
  IdentifierExpr(const std::string &name, FilePos locus);

  virtual void dump() override;
  virtual llvm::Value *getVal(ContextHolder holder) override;
  virtual Type *getType(ContextHolder holder) override;
  virtual llvm::Value *getRef(ContextHolder holder) override;
  virtual code::TreeCode getCode() const override;

private:
  std::string m_name;
};

// FIXME: maybe we should do type deduction here instead!
// The parser parse enough type so that this won't be a problem
class MemberAccessExpression : public LocatorExpression {
public:
  MemberAccessExpression(const std::string &name, const std::string &member,
                         FilePos locus);

  // from nested postfix-expression
  MemberAccessExpression(LocatorExpression *parent, const std::string &member,
                         FilePos locus);

  virtual void dump() override;
  virtual llvm::Value *getVal(ContextHolder holder) override;
  virtual llvm::Value *getRef(ContextHolder holder) override;
  virtual Type *getType(ContextHolder holder) override;
  virtual code::TreeCode getCode() const override;

  llvm::Value *getCurrentRef(ContextHolder holder);

  Type *getGEPType(ContextHolder holder);
  Type *getGEPChildType(ContextHolder holder);

  void setChildPosfixExpression(LocatorExpression *child);

private:
  // either we have a m_base_name for symbol lookup or we must have a parent
  // expression
  LocatorExpression *m_parent = nullptr, *m_child_posfix_expression = nullptr;
  std::string m_base_name; // only used when m_parent == nullptr
  std::string m_member;    // the member we are accessing
};

// FIXME: maybe we should do type deduction here instead!
class ArrayAccessExpression : public LocatorExpression {
public:
  ArrayAccessExpression(const std::string &name, Expression *expression,
                        FilePos locus);
  ArrayAccessExpression(LocatorExpression *parent, Expression *expression,
                        FilePos locus);

  virtual void dump() override;
  virtual llvm::Value *getVal(ContextHolder holder) override;
  virtual llvm::Value *getRef(ContextHolder holder) override;
  virtual code::TreeCode getCode() const override;

  llvm::Value *getCurrentRef(ContextHolder holder);

  Type *getGEPType(ContextHolder holder);
  Type *getGEPChildType(ContextHolder holder);
  virtual Type *getType(ContextHolder holder) override;

  void setChildPosfixExpression(LocatorExpression *child);

private:
  ///=== CODEGEN Options ====
  Expression *m_index_expression; // the index number
  // either we have a m_base_name for symbol lookup or we must have a parent
  // expression
  std::string m_base_name; //
  LocatorExpression *m_parent_expression = nullptr,
                    *m_child_posfix_expression; // the member we are accessing
};

// This is a weird expression
// because this both define a reference and a value
//
// int a = deref<b>; # `getVal` returns the value of the pointer
// deref<a> = 10 #  `` return the address of the pointee
class DeRefExpression : public LocatorExpression {
public:
  DeRefExpression(Expression *ref_get, FilePos locus);

  virtual void dump() override;
  virtual llvm::Value *getVal(ContextHolder holder) override;
  virtual llvm::Value *getRef(ContextHolder holder) override;
  virtual Type *getType(ContextHolder holder) override;
  virtual code::TreeCode getCode() const override;

  llvm::Value *getCurrentRef(ContextHolder holder);
  Type *getInnerType(ContextHolder holder);

  void setPosfixChildExpression(LocatorExpression *expression);

private:
  Expression *m_ref;
  LocatorExpression *m_posfix_child =
      nullptr; // it is possible for this to be the parent
               // of a posfix expression. deref<a>.a.c[10] = 10; # for example
};

// This is similar to DeRefExpression, but does the opposite thing
//
// int c = 10;
// ptr int a = ref<c>; # this makes well form
class RefExpression : public LocatorExpression {
public:
  RefExpression(Expression *inner, FilePos locus);

  virtual void dump() override;
  virtual llvm::Value *getVal(ContextHolder holder) override;
  virtual llvm::Value *getRef(ContextHolder holder) override;
  virtual Type *getType(ContextHolder holder) override;
  virtual code::TreeCode getCode() const override;

private:
  Expression *m_inner_expression;
};

// For the following:
//
// ptr char some_string = "this is my first string";
class StringLiteral : public Expression {
public:
  StringLiteral(std::string string, FilePos locus);

  virtual void dump() override;
  virtual llvm::Value *getVal(ContextHolder holder) override;
  virtual Type *getType(ContextHolder holder) override;
  virtual code::TreeCode getCode() const override;

private:
  std::string m_string_literal;
};
} // namespace vcc

#endif
