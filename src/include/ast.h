#ifndef AST_H
#define AST_H

#include <llvm-14/llvm/IR/Attributes.h>
#include <llvm/IR/Type.h>
#include <string>
#include <vector>

// llvm includes
#include "llvm/IR/Function.h"
#include "llvm/IR/Value.h"

#include "context.h"
#include "lex.h"

// defined in type.h
struct TypeInfo;
class FunctionDecl;

class ASTBase {
public:
  virtual llvm::Value *codegen(ContextHolder holder);
  virtual void dump();

  ASTBase(const std::vector<ASTBase *> childrens);

  // nullptr on failure
  const FunctionDecl *getFirstFunctionDecl() const;
  // gets the first ASTBase that defines a scope
  // nullptr on failure, get the first ASTBase that represents a scope
  const ASTBase *getScopeDeclLoc() const;
  static bool doesDefineScope(const ASTBase *at);

  const ASTBase *getParent() const;
  const std::set<ASTBase *> &getChildren() const;
  void debugDump(int depth = 1);

protected:
  void setParent(ASTBase *parent);
  void addChildren(ASTBase *children);
  void removeChildren(ASTBase *children);

private:
  ASTBase *m_parent;
  std::string m_name;
  std::set<ASTBase *> m_childrens;
};

//============================== Miscellaneous ==============================
class FunctionArgLists : public ASTBase {
public:
  using ArgsIter = std::vector<TypeInfo>::const_iterator;

  FunctionArgLists(std::vector<TypeInfo> &&args);

  // the first few alloc, and load instruction
  virtual llvm::Value *codegen(ContextHolder holder) override;

  ArgsIter begin() const;
  ArgsIter end() const;

private:
  std::vector<TypeInfo> m_args;
};

class FunctionDecl : public ASTBase {
public:
  FunctionDecl(std::vector<ASTBase *> &expression, FunctionArgLists *arg_list,
               std::string &&name, Type *return_type);

  virtual llvm::Value *codegen(ContextHolder holder) override;
  void dump() override;

  const std::string &getName() const;
  llvm::Function *getLLVMFunction() const;
  Type *getReturnType() const;

private:
  Type *m_return_type;
  std::vector<ASTBase *> m_statements;
  FunctionArgLists *m_arg_list;
  std::string m_name;

  // nullptr before codegen
  llvm::Function *m_function = nullptr;
};

//============================== Statements ==============================
class AssignmentStatement : public ASTBase {
public:
  AssignmentStatement(ASTBase *ref_expression, ASTBase *expression);

  virtual llvm::Value *codegen(ContextHolder holder) override;
  virtual void dump() override;
  const std::string &getName();

private:
  ASTBase *m_ref_expr; // The right hand side of the equation
  ASTBase *m_expression;
};

class ReturnStatement : public ASTBase {
public:
  // returning an identifier
  ReturnStatement(ASTBase *expression);

  virtual llvm::Value *codegen(ContextHolder holder) override;

private:
  // this gives some sort of value
  ASTBase *m_expression;
};

class IfStatement : public ASTBase {
public:
  IfStatement(ASTBase *cond, std::vector<ASTBase *> &&expressions);
  virtual void dump() override;
  virtual llvm::Value *codegen(ContextHolder holder) override;

private:
  // m_cond is a expression which may or may not be i1.
  // this is a terrible name
  ASTBase *m_cond;
  std::vector<ASTBase *> m_expressions;
};

class DeclarationStatement : public ASTBase {
public:
  // if expression is nullptr, it means that we just allocate space
  // and don't assign it to the thing
  DeclarationStatement(const std::string &name, ASTBase *expression,
                       Type *type);
  virtual void dump() override;
  virtual llvm::Value *codegen(ContextHolder holder) override;

private:
  std::string m_name;
  ASTBase *m_expression;
  Type *m_type;
};

class WhileStatement : public ASTBase {
public:
  WhileStatement(ASTBase *cond, std::vector<ASTBase *> &&expressions);
  virtual llvm::Value *codegen(ContextHolder holder) override;
  virtual void dump() override;

private:
  ASTBase *m_cond;
  std::vector<ASTBase *> m_expressions;
};
//============================== Expressions ==============================
// These are expressions that yields some sort of value
class Expression {
public:
  virtual Type *getType(ContextHolder holder) = 0;
};

class ConstantExpr : public ASTBase, Expression {
public:
  explicit ConstantExpr(int value);
  virtual void dump() override;
  virtual llvm::Value *codegen(ContextHolder holder) override;

  virtual Type *getType(ContextHolder holder) override;

  int getValue();

private:
  int m_value;
};

class IdentifierExpr : public ASTBase, Expression {
public:
  /// Create an identifier expression
  ///
  /// name - the name of the identifier/variable
  /// compute_ref - true if codegen returns an address, otherwise returns the
  /// value to the identifier
  IdentifierExpr(const std::string &name, bool compute_ref = false);

  virtual void dump() override;
  virtual llvm::Value *codegen(ContextHolder holder) override;

  virtual Type *getType(ContextHolder holder) override;

private:
  bool m_compute_ref;
  std::string m_name;
};

class CallExpr : public ASTBase, Expression {
public:
  CallExpr(const std::string &name, const std::vector<ASTBase *> &expressions);
  llvm::Value *codegen(ContextHolder holder) override;
  void dump() override;

  virtual Type *getType(ContextHolder holder) override;

private:
  std::string m_func_name;
  std::vector<ASTBase *> m_expressions;
};

class ParenthesesExpression : public ASTBase, Expression {
public:
  ParenthesesExpression(ASTBase *child);

  virtual Type *getType(ContextHolder holder) override;
private:
  ASTBase *m_child;
};

class BinaryExpression : public ASTBase, Expression {
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

public:
  BinaryExpression(ASTBase *lhs, BinaryExpressionType type);

  virtual void dump() override;
  virtual llvm::Value *codegen(ContextHolder holder) override;

  void setRHS(ASTBase *rhs);

private:
  ASTBase *m_lhs;
  ASTBase *m_rhs;
  BinaryExpressionType m_kind;
};

// This is a virtual base class to be inherited from
// so that Derived::codegen can use getRef from parent
// to generate code!
class RefYieldExpression : public ASTBase {
public:
  RefYieldExpression(const std::vector<ASTBase *> &childrens);

  /// The type of the child, null pointer if not found or error
protected:
  virtual llvm::Value *getRef(ContextHolder holder);

  friend class MemberAccessExpression;
  friend class ArrayAccessExpresion;
};

// FIXME: maybe we should do type deduction here instead!
// The parser parse enough type so that this won't be a problem
class MemberAccessExpression : public RefYieldExpression, Expression {
public:
  MemberAccessExpression(const std::string &name, const std::string &member,
                         bool compute_ref);
  // from nested postfix-expression
  MemberAccessExpression(RefYieldExpression *parent, const std::string &member,
                         bool compute_ref);

  virtual void dump() override;
  virtual llvm::Value *codegen(ContextHolder holder) override;
  virtual llvm::Value *getRef(ContextHolder holder) override;

  virtual Type *getType(ContextHolder holder) override;
  Type *getChildType(ContextHolder holder);

  void setChildPosfixExpression(RefYieldExpression *child);

private:
  ///=== CODEGEN Options ====
  // This is ignored when PosfixExpression::getRef or PosfixExpression::getValue
  // is called
  bool m_compute_ref;

  // either we have a m_base_name for symbol lookup or we must have a parent
  // expression
  RefYieldExpression *m_parent = nullptr, *m_child_posfix_expression = nullptr;
  std::string m_base_name; // only used when m_kind == First
  std::string m_member;    // the member we are accessing
};

// FIXME: maybe we should do type deduction here instead!
class ArrayAccessExpresion : public RefYieldExpression, Expression {
public:
  ArrayAccessExpresion(const std::string &name, ASTBase *expression,
                       bool compute_ref);
  ArrayAccessExpresion(RefYieldExpression *parent, ASTBase *expression,
                       bool compute_ref);

  virtual void dump() override;
  virtual llvm::Value *codegen(ContextHolder holder) override;
  virtual llvm::Value *getRef(ContextHolder holder) override;

  virtual Type *getType(ContextHolder holder) override;
  Type *getChildType(ContextHolder holder);

  void setChildPosfixExpression(RefYieldExpression *child);

private:
  ///=== CODEGEN Options ====
  // This is ignored when PosfixExpression::getRef or PosfixExpression::getValue
  // is called
  bool m_compute_ref,
      m_has_base_name = false; // FIXME: I don't think we really need this. This
                               // is only used for to find Type*

  ASTBase *m_index_expression; // the index number
  // either we have a m_base_name for symbol lookup or we must have a parent
  // expression
  std::string m_base_name; //
  RefYieldExpression *m_parent_expression = nullptr,
                     *m_child_posfix_expression; // the member we are accessing
};

#endif
