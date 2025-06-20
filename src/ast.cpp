#include "ast.h"
#include "util.h"

#include <cassert>
#include <iostream>
#include <llvm/IR/Constant.h>
#include <llvm/IR/DerivedTypes.h>

static void printSpaceBasedOnDepth(int depth) {
  for (int i = 0; i < depth * 2 - 1; ++i) {
    std::cout << " ";
  }
}

void ASTBase::debugDump(int depth) {
  printSpaceBasedOnDepth(depth - 1);
  std::cout << getASTClassName(this) << " ";
  dump();

  std::cout << "\n";
  for (ASTBase *children : m_childrens) {
    printSpaceBasedOnDepth(depth);

    children->debugDump(depth + 1);
  }
}

ASTBase::ASTBase(const std::vector<ASTBase *> childrens)
    : m_parent(nullptr), m_childrens() {

  for (ASTBase *children : childrens) {
    addChildren(children);
    children->setParent(this);
  }
}

void ASTBase::removeChildren(ASTBase *children) {
  assert(m_childrens.find(children) != m_childrens.end() &&
         "must contain element to begin with");

  // FIXME: we may have just leaked memory here!
  children->setParent(nullptr);
  m_childrens.erase(children);
}

void ASTBase::addChildren(ASTBase *children) {
  m_childrens.insert(children);
  children->m_parent = this;
}

void ASTBase::setParent(ASTBase *parent) {
  m_parent = parent;
  m_parent->m_childrens.insert(this);
}

ASTBase *ASTBase::getParent() { return m_parent; }

void ASTBase::dump() { return; }

const std::string &FunctionDecl::getName() const { return m_name; }

llvm::Function *FunctionDecl::getLLVMFunction() const { return m_function; }

FunctionDecl::FunctionDecl(std::vector<ASTBase *> &statements,
                           FunctionArgLists *arg_list, std::string &&name)
    : ASTBase(statements), m_statements(statements), m_arg_list(arg_list),
      m_name(name) {}

FunctionArgLists::FunctionArgLists(std::vector<TypeInfo> &&args)
    : m_args(args) {}

FunctionArgLists::ArgsIter FunctionArgLists::begin() const {
  return m_args.cbegin();
}

FunctionArgLists::ArgsIter FunctionArgLists::end() const {
  return m_args.cend();
}

AssignmentStatement::AssignmentStatement(const std::string &name,
                                         ASTBase *expression)
    : ASTBase({expression}), m_name(name), m_expression(expression) {}

const std::string &AssignmentStatement::getName() { return m_name; }

void FunctionDecl::dump() { std::cout << "name: " << m_name; }

ReturnStatement::ReturnStatement(ASTBase *expression)
    : ASTBase({expression}), m_expression(expression) {}

IdentifierExpr::IdentifierExpr(const std::string &name)
    : ASTBase({}), m_name(name) {}

ConstantExpr::ConstantExpr(int value) : ASTBase({}), m_value(value) {}

int ConstantExpr::getValue() { return m_value; }

ParenthesesExpression::ParenthesesExpression(ASTBase *child)
    : ASTBase({child}), m_child(child) {}

BinaryExpression::BinaryExpression(ASTBase *lhs, BinaryExpressionType type)
    : ASTBase({lhs}), m_lhs(lhs), m_rhs(nullptr), m_kind(type) {}

BinaryExpression::BinaryExpressionType
BinaryExpression::getFromLexType(lex::Token token) {
  switch (token.getType()) {
  case lex::Add:
    return Add;
  case lex::Multiply:
    return Multiply;
  default:
    assert(false && "invalid token type");
    return Add;
  }
}

void BinaryExpression::setRHS(ASTBase *rhs) {
  assert(!m_rhs && "expected it to be a null pointer");
  addChildren(rhs);

  m_rhs = rhs;
}

void BinaryExpression::dump() {
  switch (m_kind) {
  case Add:
    std::cout << "+";
    break;
  case Multiply:
    std::cout << "*";
    break;
  default:
    std::cout << "unknown";
    break;
  }

  return;
}

void IdentifierExpr::dump() { std::cout << "identifier: " << m_name; }

void ConstantExpr::dump() { std::cout << m_value; }

FunctionDecl *ASTBase::getFirstFunctionDecl() {
  for (ASTBase *current = this; current; current = current->getParent()) {
    FunctionDecl *decl = nullptr;
    if ((decl = dynamic_cast<FunctionDecl *>(current)))
      return decl;
  }

  return nullptr;
}

// ======================================================
// ====================== CODE GEN ======================
llvm::Value *ASTBase::codegen(ContextHolder holder) {
  assert(false && "please implement codegen");
  return nullptr;
}

llvm::Value *ConstantExpr::codegen(ContextHolder holder) {
  llvm::Value *value =
      llvm::ConstantInt::get(llvm::Type::getInt32Ty(holder->context), m_value);
  return value;
}

llvm::Value *BinaryExpression::codegen(ContextHolder holder) {
  llvm::Value *right_hand_side = m_rhs->codegen(holder);
  llvm::Value *left_hand_side = m_lhs->codegen(holder);

  assert(right_hand_side && left_hand_side && "cannot be null");
  switch (m_kind) {
  case Add: {
    llvm::Value *result =
        holder->builder.CreateAdd(left_hand_side, right_hand_side);
    return result;
  }
  case Multiply: {
    llvm::Value *reuslt =
        holder->builder.CreateMul(left_hand_side, right_hand_side);
    return reuslt;
  }
  default:
    assert(false && "cannot get here");
    return nullptr;
  }

  assert(false && "how did we get here");
  return nullptr;
}

llvm::Value *IdentifierExpr::codegen(ContextHolder holder) {

  // this is usually a pointer
  // FIXME: it seems that we need to encode more type
  // information
  llvm::Value *loc_value =
      holder->symbol_table.lookupLocalVariable(getFirstFunctionDecl(), m_name);
  llvm::Value *value = holder->builder.CreateLoad(
      llvm::Type::getInt32Ty(holder->context), loc_value);

  return value;
}

void FunctionArgLists::codegen(ContextHolder holder, FunctionDecl *func) {
  // appending to symbol table
  int count = 0;
  llvm::Function *llvm_function = func->getLLVMFunction();
  for (llvm::Argument &arg : llvm_function->args()) {
    const std::string &name = m_args[count++].name;
    arg.setName(name);

    // allocating one integer
    llvm::Value *alloc_loc =
        holder->builder.CreateAlloca(llvm::Type::getInt32Ty(holder->context));
    holder->builder.CreateStore(&arg, alloc_loc);

    holder->symbol_table.addLocalVariable(func, name, alloc_loc);
  }
}

llvm::Value *AssignmentStatement::codegen(ContextHolder holder) {
  llvm::Value *expression_val = m_expression->codegen(holder);
  assert(expression_val && "must yield a non negative result");
  llvm::Value *alloc_loc =
      holder->symbol_table.lookupLocalVariable(getFirstFunctionDecl(), m_name);

  holder->builder.CreateStore(expression_val, alloc_loc);

  return nullptr;
}

llvm::Value *ReturnStatement::codegen(ContextHolder holder) {
  // assert(false && "I made it here somehow");
  llvm::Value *return_value = m_expression->codegen(holder);
  assert(return_value && "expression must return a value");
  holder->builder.CreateRet(return_value);

  return nullptr;
}

llvm::Value *FunctionDecl::codegen(ContextHolder holder) {
  // FIXME: make this more efficient
  std::vector<llvm::Type *> args;
  for (auto it = m_arg_list->begin(), ie = m_arg_list->end(); it != ie; ++it) {
    args.push_back(llvm::Type::getInt32Ty(holder->context));
  }

  llvm::FunctionType *function_type = llvm::FunctionType::get(
      llvm::Type::getInt32Ty(holder->context), args, /*isVarArg=*/false);

  m_function = llvm::Function::Create(
      function_type, llvm::Function::ExternalLinkage, m_name, holder->module);

  // generating code for something
  llvm::BasicBlock *block =
      llvm::BasicBlock::Create(holder->context, "main", m_function);
  holder->builder.SetInsertPoint(block);

  // copy the parameter into llvm ir
  m_arg_list->codegen(holder, this);

  // code generation for statement
  for (ASTBase *statement : m_statements) {
    statement->codegen(holder);
  }

  // add this to symbol table
  holder->symbol_table.addFunction(this);
  return m_function;
}
