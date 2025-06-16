#ifndef AST_H
#define AST_H

#include <string>
#include <vector>

// llvm includes
#include "llvm/IR/Function.h"
#include "llvm/IR/Value.h"

#include "context.h"
#include "lex.h"

class ASTBase {
public:
  virtual llvm::Value *codegen(ContextHolder holder);
  virtual void dump();

private:
};

enum Type { Int32 };

struct TypeInfo {
  TypeInfo() = default;

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
  void codegen(ContextHolder holder, llvm::Function *func);

  ArgsIter begin() const;
  ArgsIter end() const;

private:
  std::vector<TypeInfo> m_args;
};

class FunctionDecl : public ASTBase {
public:
  FunctionDecl(std::vector<ASTBase *> &&expression, FunctionArgLists *arg_list,
               std::string &&name);

  virtual llvm::Value *codegen(ContextHolder holder) override;
  void dump() override;

private:
  std::vector<ASTBase *> m_statements;
  FunctionArgLists *m_arg_list;
  std::string m_name;

  llvm::Function *m_function = nullptr;
};

//============================== Statements ==============================
enum AssignmentType {
  Constant,
};

class AssignmentStatement : public ASTBase {
public:
  AssignmentStatement(const std::string &name, long long value);

  virtual llvm::Value *codegen(ContextHolder holder);
  const std::string &getName();
  long long getValue();

private:
  std::string m_name;
  // valid when assignment type is set to Constant
  long long m_value;
  AssignmentType m_type = Constant;
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
  virtual llvm::Value* codegen(ContextHolder holder) override; 

  int getValue();

private:
  int m_value;
};

class IdentifierExpr : public ASTBase {
public:
  IdentifierExpr(const std::string &name);

  virtual void dump() override; 
  virtual llvm::Value* codegen(ContextHolder holder) override; 

private:
  std::string m_name;
};

class CallExpr : public ASTBase {
public:
  CallExpr(const std::string &name);

private:
  std::string m_func_name;
};

class ParenthesesExpression : public ASTBase {
public:
  ParenthesesExpression(ASTBase *child);

private:
  ASTBase *m_child;
};

class BinaryExpression : public ASTBase {
public:
  enum BinaryExpressionType { Add, Multiply };
  static BinaryExpressionType getFromLexType(lex::Token lex_type);

public:
  BinaryExpression(ASTBase *lhs, BinaryExpressionType type);

  virtual void dump() override;
  virtual llvm::Value* codegen(ContextHolder holder) override;

  void setRHS(ASTBase *rhs);

private:
  ASTBase *m_lhs;
  ASTBase *m_rhs;
  BinaryExpressionType m_kind;
};

#endif
