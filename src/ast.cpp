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

const std::set<ASTBase *> &ASTBase::getChildren() const { return m_childrens; }

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

ASTBase *ASTBase::getParent() const { return m_parent; }

void ASTBase::dump() { return; }

const std::string &FunctionDecl::getName() const { return m_name; }

llvm::Function *FunctionDecl::getLLVMFunction() const { return m_function; }

FunctionDecl::FunctionDecl(std::vector<ASTBase *> &statements,
                           FunctionArgLists *arg_list, std::string &&name, Type* ret)
    : ASTBase({arg_list}), m_statements(statements), m_arg_list(arg_list),
      m_name(name), m_return_type(ret) {
  // making sure that arg_list is always the first in the syntax tree!
  for(ASTBase* statement: statements){
      addChildren(statement);
  }
}

FunctionArgLists::FunctionArgLists(std::vector<TypeInfo> &&args)
    : ASTBase({}), m_args(args) {}

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

void FunctionDecl::dump() {
  std::cout << "name: " << m_name << " args: ";
  for (auto it = m_arg_list->begin(), ie = m_arg_list->end(); it != ie; ++it) {
    std::cout << it->name << ", ";
  }
}

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
  case lex::Subtract:
    return Subtract;
  case lex::Multiply:
    return Multiply;
  case lex::EqualKeyword:
    return Equal;
  case lex::NEquals:
    return NEquals;
  case lex::GreaterEqual:
    return GE;
  case lex::GreaterThan:
    return GT;
  case lex::LessEqual:
    return LE;
  case lex::LessThan:
    return LT;
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
  case Equal:
    std::cout << "equals";
    break;
  default:
    std::cout << "unknown";
    break;
  }

  return;
}

void IdentifierExpr::dump() { std::cout << "identifier: " << m_name; }

void ConstantExpr::dump() { std::cout << m_value; }

ASTBase *ASTBase::getScopeDeclLoc() const {
  ASTBase *parent = getParent();
  while (parent && !doesDefineScope(parent)) {
    parent = parent->getParent();
  }

  return parent;
}

bool ASTBase::doesDefineScope(ASTBase *at) {
  return isa<FunctionDecl>(at) || isa<IfStatement>(at) ||
         isa<WhileStatement>(at);
}

FunctionDecl *ASTBase::getFirstFunctionDecl() {
  for (ASTBase *current = this; current; current = current->getParent()) {
    FunctionDecl *decl = nullptr;
    if ((decl = dynamic_cast<FunctionDecl *>(current)))
      return decl;
  }

  return nullptr;
}

CallExpr::CallExpr(const std::string &name,
                   const std::vector<ASTBase *> &expression)
    : ASTBase(expression), m_func_name(name), m_expressions(expression) {}

void CallExpr::dump() { std::cout << "name: " << m_func_name; }

IfStatement::IfStatement(ASTBase *cond, std::vector<ASTBase *> &&expressions)
    : ASTBase({cond}), m_cond(cond), m_expressions(expressions) {
  for (ASTBase *expression : expressions) {
    addChildren(expression);
  }
}

void IfStatement::dump() {}

DeclarationStatement::DeclarationStatement(const std::string &name,
                                           ASTBase *base, Type* type)
    : ASTBase({base}), m_expression(base), m_name(name), m_type(type) {}

void DeclarationStatement::dump() { std::cout << "name: " << m_name; }

WhileStatement::WhileStatement(ASTBase *cond,
                               std::vector<ASTBase *> &&expression)
    : ASTBase({cond}), m_cond(cond), m_expressions(expression) {
  for (ASTBase *base : m_expressions) {
    addChildren(base);
  }
}

void WhileStatement::dump() { return; }

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

  assert(right_hand_side->getType()->isIntegerTy() &&
         left_hand_side->getType()->isIntegerTy() &&
         "both side most be integer for now");

  // See Binary comparison rule
  if (right_hand_side->getType()->getPrimitiveSizeInBits() >
      left_hand_side->getType()->getPrimitiveSizeInBits()) {
    left_hand_side =
        holder->builder.CreateSExt(left_hand_side, right_hand_side->getType());
  } else if (left_hand_side->getType()->getPrimitiveSizeInBits() >
             right_hand_side->getType()->getPrimitiveSizeInBits()) {
    right_hand_side =
        holder->builder.CreateSExt(right_hand_side, left_hand_side->getType());
  }

  assert(right_hand_side && left_hand_side && "cannot be null");
  assert(left_hand_side->getType() == right_hand_side->getType());
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
  case Equal: {
    left_hand_side->getType();
    llvm::Value *equal_check =
        holder->builder.CreateICmpEQ(left_hand_side, right_hand_side);
    return equal_check;
  }
  case NEquals: {
    llvm::Value *not_equal_check =
        holder->builder.CreateICmpNE(left_hand_side, right_hand_side);
    return not_equal_check;
  }
  case GE: {
    llvm::Value *check =
        holder->builder.CreateICmpSGE(left_hand_side, right_hand_side);
    return check;
  }
  case GT: {
    llvm::Value *check =
        holder->builder.CreateICmpSGT(left_hand_side, right_hand_side);
    return check;
  }
  case Subtract: {
    llvm::Value *subtract =
        holder->builder.CreateSub(left_hand_side, right_hand_side);
    return subtract;
  }
  case LE: {
    llvm::Value *check =
        holder->builder.CreateICmpSLE(left_hand_side, right_hand_side);
    return check;
  }
  case LT: {
    llvm::Value *check =
        holder->builder.CreateICmpSLT(left_hand_side, right_hand_side);
    return check;
  }
  default:
    assert(false && "cannot get here");
    return nullptr;
  }

  assert(false && "how did we get here");
  return nullptr;
}

llvm::Value *IdentifierExpr::codegen(ContextHolder holder) {
  // FIXME: it seems that we need to encode more type
  // information
  llvm::Value *loc_value =
      holder->symbol_table.lookupLocalVariable(this, m_name);
  llvm::Value *value = holder->builder.CreateLoad(
      llvm::Type::getInt32Ty(holder->context), loc_value);

  return value;
}

llvm::Value *FunctionArgLists::codegen(ContextHolder holder) {
  FunctionDecl *func = getFirstFunctionDecl();

  // appending to symbol table
  int count = 0;
  llvm::Function *llvm_function = func->getLLVMFunction();
  for (llvm::Argument &arg : llvm_function->args()) {
    const std::string &name = m_args[count++].name;
    arg.setName(name);

    // allocating one integer
    llvm::Value *alloc_loc =
        holder->builder.CreateAlloca(arg.getType());
    holder->builder.CreateStore(&arg, alloc_loc);

    holder->symbol_table.addLocalVariable(this, name, alloc_loc);
  }

  return nullptr;
}

llvm::Value *AssignmentStatement::codegen(ContextHolder holder) {
  llvm::Value *expression_val = m_expression->codegen(holder);
  assert(expression_val && "must yield a non negative result");
  llvm::Value *alloc_loc =
      holder->symbol_table.lookupLocalVariable(this, m_name);

  holder->builder.CreateStore(expression_val, alloc_loc);

  return nullptr;
}
void AssignmentStatement::dump() { std::cout << "name: " << m_name; }

llvm::Value *ReturnStatement::codegen(ContextHolder holder) {
  // FIXME: must add semantics analysis
  llvm::Value *return_value = m_expression->codegen(holder);
  assert(return_value && "expression must return a value");
  assert(return_value->getType()->isIntegerTy() &&
         "must be integer type for now");

  // sign extend value for now!
  if (!return_value->getType()->isIntegerTy(32)) {
    return_value = holder->builder.CreateSExt(
        return_value, llvm::Type::getInt32Ty(holder->context));
  }

  holder->builder.CreateRet(return_value);

  return nullptr;
}

llvm::Value *FunctionDecl::codegen(ContextHolder holder) {
  // FIXME: make this more efficient
  std::vector<llvm::Type *> args;
  for (auto it = m_arg_list->begin(), ie = m_arg_list->end(); it != ie; ++it) {
    args.push_back(it->type->getType(holder));
  }

  llvm::FunctionType *function_type = llvm::FunctionType::get(
      m_return_type->getType(holder), args, /*isVarArg=*/false);

  m_function = llvm::Function::Create(
      function_type, llvm::Function::ExternalLinkage, m_name, holder->module);
  m_function->setDSOLocal(true);

  // add this to symbol table
  holder->symbol_table.addFunction(this);

  // generating code for something
  llvm::BasicBlock *block =
      llvm::BasicBlock::Create(holder->context, "", m_function);
  holder->builder.SetInsertPoint(block);

  // code generation for statement
  m_arg_list->codegen(holder);
  for (ASTBase *statement : m_statements) {
    statement->codegen(holder);
  }

  return m_function;
}

llvm::Value *CallExpr::codegen(ContextHolder holder) {
  llvm::Function *function = holder->symbol_table.lookupFunction(m_func_name);
  assert(function && "this must exist for codegen!");
  assert(function->arg_size() == m_expressions.size() &&
         "expected the same number of argument");
  std::vector<llvm::Value *> args;
  for (ASTBase *expression : m_expressions) {
    args.push_back(expression->codegen(holder));
  }

  llvm::Value *result =
      holder->builder.CreateCall(function->getFunctionType(), function, args);

  return result;
}

llvm::Value *IfStatement::codegen(ContextHolder holder) {
  llvm::Function *function = getFirstFunctionDecl()->getLLVMFunction();
  llvm::BasicBlock *true_if_block =
      llvm::BasicBlock::Create(holder->context, "", function);
  llvm::BasicBlock *fallthrough_block =
      llvm::BasicBlock::Create(holder->context, "", function);

  llvm::Value *cond = m_cond->codegen(holder);
  assert(cond->getType()->isIntegerTy() && "must be integer type");
  cond = holder->builder.CreateICmpNE(
      cond, llvm::ConstantInt::get(cond->getType(), 0));

  llvm::Value *stuff =
      holder->builder.CreateCondBr(cond, true_if_block, fallthrough_block);

  holder->builder.SetInsertPoint(true_if_block);
  for (ASTBase *expression : m_expressions) {
    expression->codegen(holder);
  }

  assert(m_expressions.size() >= 1 && "must be true for now");
  ASTBase *last_expression = m_expressions[m_expressions.size() - 1];
  if (dynamic_cast<ReturnStatement *>(last_expression) == nullptr)
    holder->builder.CreateBr(fallthrough_block);

  holder->builder.SetInsertPoint(fallthrough_block);

  return nullptr;
}

llvm::Value *DeclarationStatement::codegen(ContextHolder holder) {
  // initialize the first variable
  FunctionDecl *func = getFirstFunctionDecl();
  llvm::Value *alloc_loc =
      holder->builder.CreateAlloca(m_type->getType(holder));
  holder->symbol_table.addLocalVariable(this, m_name, alloc_loc);

  llvm::Value *exp = m_expression->codegen(holder);
  llvm::Value *return_val = holder->builder.CreateStore(exp, alloc_loc);
  return return_val;
}

llvm::Value *WhileStatement::codegen(ContextHolder holder) {
  llvm::BasicBlock *cond_block = llvm::BasicBlock::Create(
      holder->context, "", getFirstFunctionDecl()->getLLVMFunction());
  llvm::BasicBlock *while_true_block = llvm::BasicBlock::Create(
      holder->context, "", getFirstFunctionDecl()->getLLVMFunction());
  llvm::BasicBlock *fallthrough = llvm::BasicBlock::Create(
      holder->context, "", getFirstFunctionDecl()->getLLVMFunction());

  holder->builder.CreateBr(cond_block);

  // set up the cond block
  holder->builder.SetInsertPoint(cond_block);
  llvm::Value *cond = m_cond->codegen(holder);
  assert(cond->getType()->isIntegerTy() && "must be integer");
  cond = holder->builder.CreateICmpNE(
      cond, llvm::ConstantInt::get(cond->getType(), 0));
  holder->builder.CreateCondBr(cond, while_true_block, fallthrough);

  // set up while body block
  holder->builder.SetInsertPoint(while_true_block);
  for (ASTBase *statement : m_expressions) {
    statement->codegen(holder);
  }
  assert(m_expressions.size() >= 1 && "must be true for now");
  ASTBase *last_statement = m_expressions[m_expressions.size() - 1];
  if (dynamic_cast<ReturnStatement *>(last_statement) == nullptr)
    holder->builder.CreateBr(cond_block);

  holder->builder.SetInsertPoint(fallthrough);

  return nullptr;
}
