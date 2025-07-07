#ifndef AST_H
#define AST_H

#include <string>
#include <vector>

// llvm includes
#include "llvm/IR/Function.h"
#include "llvm/IR/Value.h"

#include "context.h"
#include "lex.h"

class FunctionDecl;

class ASTBase {
public:
  virtual llvm::Value *codegen(ContextHolder holder);
  virtual void dump();

  ASTBase(const std::vector<ASTBase *> childrens);

  // nullptr on failure
  FunctionDecl *getFirstFunctionDecl();

  ASTBase *getParent();
  void setParent(ASTBase *parent);
  void addChildren(ASTBase *children);
  void removeChildren(ASTBase *children);

  // depth = 1 is start
  void debugDump(int depth = 1);
  const std::string &getName();

private:
  ASTBase *m_parent;
  std::string m_name;
  std::set<ASTBase *> m_childrens;
};

enum Type { Int32 };

struct TypeInfo {

  Type kind;
  std::string name;
};

//============================== Miscellaneous ==============================

// FIXME: remove this class in the future. This makes
// this makes codegen have non trivial behavior!
// We are already expected a function declaration in the LLVM IR
// level already, so what is the point of this class?
// FIXME: maybe put this in private class?
class FunctionArgLists {
public:
  using ArgsIter = std::vector<TypeInfo>::const_iterator;

  FunctionArgLists(std::vector<TypeInfo> &&args);

  // the first few alloc, and load instruction
  void codegen(ContextHolder holder, FunctionDecl *function_decl);

  ArgsIter begin() const;
  ArgsIter end() const;

private:
  std::vector<TypeInfo> m_args;
};

class FunctionDecl : public ASTBase {
public:
  FunctionDecl(std::vector<ASTBase *> &expression, FunctionArgLists *arg_list,
               std::string &&name);

  virtual llvm::Value *codegen(ContextHolder holder) override;
  void dump() override;

  const std::string &getName() const;
  llvm::Function *getLLVMFunction() const;

private:
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

  virtual llvm::Value *codegen(ContextHolder holder);
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
  enum BinaryExpressionType { Add, Multiply, Equal };
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

#endif
