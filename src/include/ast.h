#ifndef AST_H
#define AST_H

#include <string>
#include <vector>

// llvm includes
#include "llvm/IR/Function.h"
#include "llvm/IR/Value.h"

#include "context.h"
#include "lex.h"
#include "type.h"

class FunctionDecl;

class ASTBase {
public:
  virtual llvm::Value *codegen(ContextHolder holder);
  virtual void dump();

  ASTBase(const std::vector<ASTBase *> childrens);

  // nullptr on failure
  FunctionDecl *getFirstFunctionDecl();
  // gets the first ASTBase that defines a scope
  // nullptr on failure, get the first ASTBase that represents a scope
  ASTBase *getScopeDeclLoc() const;
  static bool doesDefineScope(ASTBase *at);

  ASTBase *getParent() const;
  void setParent(ASTBase *parent);
  void addChildren(ASTBase *children);
  void removeChildren(ASTBase *children);
  const std::set<ASTBase *> &getChildren() const;

  // depth = 1 is start
  void debugDump(int depth = 1);

private:
  ASTBase *m_parent;
  std::string m_name;
  std::set<ASTBase *> m_childrens;
};

// FIXME: remove me
struct TypeInfo {
  Type *type;
  std::string name;
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
  AssignmentStatement(const std::string &name, ASTBase *expression);

  virtual llvm::Value *codegen(ContextHolder holder) override;
  virtual void dump() override;
  const std::string &getName();

private:
  std::string m_name;
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

class ConstantExpr : public ASTBase {
public:
  explicit ConstantExpr(int value);
  virtual void dump() override;
  virtual llvm::Value *codegen(ContextHolder holder) override;

  int getValue();

private:
  int m_value;
};

class IdentifierExpr : public ASTBase {
public:
  IdentifierExpr(const std::string &name);

  virtual void dump() override;
  virtual llvm::Value *codegen(ContextHolder holder) override;

private:
  std::string m_name;
};

class CallExpr : public ASTBase {
public:
  CallExpr(const std::string &name, const std::vector<ASTBase *> &expressions);
  llvm::Value *codegen(ContextHolder holder) override;
  void dump() override;

private:
  std::string m_func_name;
  std::vector<ASTBase *> m_expressions;
};

class ParenthesesExpression : public ASTBase {
public:
  ParenthesesExpression(ASTBase *child);

private:
  ASTBase *m_child;
};

class BinaryExpression : public ASTBase {
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
  };
  static BinaryExpressionType getFromLexType(lex::Token lex_type);

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

class MemberAccessExpression : public ASTBase {
public:
  MemberAccessExpression(const std::vector<std::string>& member_access_order);

  virtual void dump() override;
  virtual llvm::Value *codegen(ContextHolder holder) override;

private:
  std::vector<std::string> m_member_accesses;
};

#endif
