#include "ast.h"
#include "type.h"
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

const ASTBase *ASTBase::getParent() const { return m_parent; }

void ASTBase::dump() { return; }

const std::string &FunctionDecl::getName() const { return m_name; }

llvm::Function *FunctionDecl::getLLVMFunction() const { return m_function; }

FunctionDecl::FunctionDecl(std::vector<ASTBase *> &statements,
                           FunctionArgLists *arg_list, std::string &&name,
                           Type *ret)
    : ASTBase({arg_list}), m_statements(statements), m_arg_list(arg_list),
      m_name(name), m_return_type(ret) {
  // making sure that arg_list is always the first in the syntax tree!
  for (ASTBase *statement : statements) {
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

AssignmentStatement::AssignmentStatement(ASTBase *ref_expr, ASTBase *expression)
    : ASTBase({ref_expr, expression}), m_ref_expr(ref_expr),
      m_expression(expression) {}

void FunctionDecl::dump() {
  std::cout << "name: " << m_name << " args: ";
  for (auto it = m_arg_list->begin(), ie = m_arg_list->end(); it != ie; ++it) {
    std::cout << it->name << ", ";
  }
}

ReturnStatement::ReturnStatement(ASTBase *expression)
    : ASTBase({expression}), m_expression(expression) {}

IdentifierExpr::IdentifierExpr(const std::string &name, bool compute_ref)
    : ASTBase({}), m_name(name), m_compute_ref(compute_ref) {}

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

void IdentifierExpr::dump() {
  std::cout << "identifier: " << m_name
            << " compute-ref: " << (m_compute_ref ? "true" : "false");
}

void ConstantExpr::dump() { std::cout << m_value; }

const ASTBase *ASTBase::getScopeDeclLoc() const {
  const ASTBase *parent = getParent();
  while (parent && !doesDefineScope(parent)) {
    parent = parent->getParent();
  }

  return parent;
}

bool ASTBase::doesDefineScope(const ASTBase *at) {
  return isa<const FunctionDecl>(at) || isa<const IfStatement>(at) ||
         isa<const WhileStatement>(at);
}

const FunctionDecl *ASTBase::getFirstFunctionDecl() const {
  for (const ASTBase *current = this; current; current = current->getParent()) {
    const FunctionDecl *decl = nullptr;
    if ((decl = dynamic_cast<const FunctionDecl *>(current)))
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
                                           ASTBase *base, Type *type)
    : ASTBase({}), m_expression(base), m_name(name), m_type(type) {
  // it is possible that the child is a nullptr, meaning we only have to
  // allocate space
  if (base)
    addChildren(base);
}

void DeclarationStatement::dump() { std::cout << "name: " << m_name; }

WhileStatement::WhileStatement(ASTBase *cond,
                               std::vector<ASTBase *> &&expression)
    : ASTBase({cond}), m_cond(cond), m_expressions(expression) {
  for (ASTBase *base : m_expressions) {
    addChildren(base);
  }
}

void WhileStatement::dump() { return; }

MemberAccessExpression::MemberAccessExpression(const std::string &name,
                                               const std::string &member,
                                               bool compute_ref)
    : m_base_name(name), m_member(member), m_compute_ref(compute_ref),
      RefYieldExpression({}) {}

MemberAccessExpression::MemberAccessExpression(RefYieldExpression *parent,
                                               const std::string &member,
                                               bool compute_ref)
    : m_member(member), RefYieldExpression({}), m_parent(parent),
      m_compute_ref(compute_ref) {
  parent->addChildren(this);
}

void MemberAccessExpression::dump() {
  // if we don't have a parent, we must have a valid m_base_name
  if (!m_parent)
    std::cout << m_base_name;

  std::cout << "." << m_member << " is_ref: " << m_compute_ref
            << " child: " << m_child_posfix_expression << " this: " << this;
}

ArrayAccessExpresion::ArrayAccessExpresion(const std::string &name,
                                           ASTBase *expression,
                                           bool compute_ref)
    : RefYieldExpression({expression}), m_has_base_name(true),
      m_index_expression(expression), m_base_name(name),
      m_compute_ref(compute_ref) {}

ArrayAccessExpresion::ArrayAccessExpresion(RefYieldExpression *parent,
                                           ASTBase *index_expression,
                                           bool compute_ref)
    : RefYieldExpression({index_expression}),
      m_index_expression(index_expression), m_parent_expression(parent),
      m_compute_ref(compute_ref) {
  parent->addChildren(this);
}

void ArrayAccessExpresion::dump() {

  std::cout << "[]"
            << " is_ref: " << (m_compute_ref)
            << " child*: " << m_child_posfix_expression << " this: " << this;
}

RefYieldExpression::RefYieldExpression(const std::vector<ASTBase *> &childrens)
    : ASTBase({childrens}) {}

Type *ArrayAccessExpresion::getChildType(ContextHolder holder) {
  Type *current_type = getType(holder);
  assert(current_type->isArray() &&
         "array access expression must have array type!");

  return current_type->getAs<ArrayType>()->getBase();
}

Type *MemberAccessExpression::getChildType(ContextHolder holder) {
  return getType(holder)
      ->getAs<StructType>()
      ->getElement(m_member)
      .value()
      .type;
}

void ArrayAccessExpresion::setChildPosfixExpression(RefYieldExpression *child) {
  m_child_posfix_expression = child;
}

void MemberAccessExpression::setChildPosfixExpression(
    RefYieldExpression *child) {
  m_child_posfix_expression = child;
}

llvm::Value *RefYieldExpression::getRef(ContextHolder holder) {
  assert(false && "please implement this!'");
}

llvm::Value *MemberAccessExpression::getRef(ContextHolder holder) {
  llvm::Value *start_of_pointer =
      (m_parent == nullptr)
          ? holder->symbol_table.lookupLocalVariable(this, m_base_name).value
          : m_parent->getRef(holder);

  // getting the actual field number
  Type *current_type = getType(holder);
  int field_num =
      current_type->getAs<StructType>()->getElement(m_member)->field_num;
  llvm::Value *zero =
      llvm::ConstantInt::get(llvm::Type::getInt32Ty(holder->context), 0);
  llvm::Value *offset = llvm::ConstantInt::get(
      llvm::Type::getInt32Ty(holder->context), field_num);

  return holder->builder.CreateGEP(current_type->getType(holder),
                                   start_of_pointer, {zero, offset});
}

llvm::Value *ArrayAccessExpresion::getRef(ContextHolder holder) {
  // if we don't have a parent, then we can get the
  // location from a symbol table lookup! It means we
  // are at the most top level! Or else we get the location from parent's getRef
  llvm::Value *start_of_pointer =
      (m_parent_expression == nullptr)
          ? holder->symbol_table.lookupLocalVariable(this, m_base_name).value
          : m_parent_expression->getRef(holder);

  llvm::Type *llvm_type = getType(holder)->getType(holder);
  llvm::ConstantInt *zero =
      llvm::ConstantInt::get(llvm::Type::getInt32Ty(holder->context), 0);
  llvm::Value *offset = m_index_expression->codegen(holder);

  return holder->builder.CreateGEP(llvm_type, start_of_pointer, {zero, offset});
}

Type *FunctionDecl::getReturnType() const { return m_return_type; }

// ================================================================================
// ====================== Expression Implementation::getType
// ======================
Type *MemberAccessExpression::getType(ContextHolder holder) {
  if (!m_parent)
    return holder->symbol_table.lookupLocalVariable(this, m_base_name).type;

  // FIXME: this shares a lot same code with ArrayAccessExpression::getType,
  // maybe we should have a standard interface that solves this entirely?
  if (ArrayAccessExpresion *parent = dyncast<ArrayAccessExpresion>(m_parent)) {
    return parent->getChildType(holder);
  }
  assert(isa<MemberAccessExpression>(m_parent));
  MemberAccessExpression *parent = dyncast<MemberAccessExpression>(m_parent);
  return parent->getChildType(holder);
}

Type *ConstantExpr::getType(ContextHolder holder) {
  return new BuiltinType(BuiltinType::Int);
}

Type *IdentifierExpr::getType(ContextHolder holder) {
  return holder->symbol_table.lookupLocalVariable(this, m_name).type;
}

Type *CallExpr::getType(ContextHolder holder) {
  return holder->symbol_table.lookupFunction(m_func_name)->getReturnType();
}

Type *ParenthesesExpression::getType(ContextHolder holder) {
  return dynamic_cast<Expression *>(m_child)->getType(holder);
}

Type *BinaryExpression::getType(ContextHolder holder) {
  assert(false && "not sure what to do this");
}

Type *ArrayAccessExpresion::getType(ContextHolder holder) {
  if (!m_parent_expression)
    return holder->symbol_table.lookupLocalVariable(this, m_base_name).type;

  // trying to get type from parent expression!
  if (ArrayAccessExpresion *parent =
          dyncast<ArrayAccessExpresion>(m_parent_expression)) {
    return parent->getChildType(holder);
  }

  assert(isa<MemberAccessExpression>(m_parent_expression) &&
         "must be member expresion beacuse we have no options left!");
  MemberAccessExpression *parent =
      dyncast<MemberAccessExpression>(m_parent_expression);
  return parent->getChildType(holder);
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
      holder->symbol_table.lookupLocalVariable(this, m_name).value;
  if (m_compute_ref)
    return loc_value;

  llvm::Value *value = holder->builder.CreateLoad(
      llvm::Type::getInt32Ty(holder->context), loc_value);
  return value;
}

llvm::Value *FunctionArgLists::codegen(ContextHolder holder) {
  const FunctionDecl *func = getFirstFunctionDecl();

  // appending to symbol table
  int count = 0;
  llvm::Function *llvm_function = func->getLLVMFunction();
  for (llvm::Argument &arg : llvm_function->args()) {
    const std::string &name = m_args[count].name;
    arg.setName(name);

    // allocating one integer
    llvm::Value *alloc_loc = holder->builder.CreateAlloca(arg.getType());
    holder->builder.CreateStore(&arg, alloc_loc);

    holder->symbol_table.addLocalVariable(this, name, m_args[count].type,
                                          alloc_loc);
    ++count;
  }

  return nullptr;
}

llvm::Value *AssignmentStatement::codegen(ContextHolder holder) {
  llvm::Value *expression_val = m_expression->codegen(holder);
  assert(expression_val && "must yield a non negative result");
  llvm::Value *alloc_loc = m_ref_expr->codegen(holder);

  holder->builder.CreateStore(expression_val, alloc_loc);

  return nullptr;
}

void AssignmentStatement::dump() {}

llvm::Value *ReturnStatement::codegen(ContextHolder holder) {
  // FIXME: must add semantics analysis
  llvm::Value *return_value = m_expression->codegen(holder);

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
  llvm::Function *function =
      holder->symbol_table.lookupFunction(m_func_name)->getLLVMFunction();
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
  const FunctionDecl *func = getFirstFunctionDecl();
  llvm::Value *alloc_loc =
      holder->builder.CreateAlloca(m_type->getType(holder));
  holder->symbol_table.addLocalVariable(this, m_name, m_type, alloc_loc);

  // if we don't have an initializer, we don't allocate space
  if (m_expression) {
    llvm::Value *exp = m_expression->codegen(holder);
    llvm::Value *return_val = holder->builder.CreateStore(exp, alloc_loc);
    return return_val;
  }

  return nullptr;
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

llvm::Value *MemberAccessExpression::codegen(ContextHolder holder) {
  if (m_child_posfix_expression)
    return m_child_posfix_expression->codegen(holder);

  // we are at the base case
  llvm::Value *ref_loc = getRef(holder);
  if (m_compute_ref)
    return ref_loc;

  llvm::Type *child_type =
      getType(holder)->getAs<StructType>()->getElement(m_member)->type->getType(
          holder);
  return holder->builder.CreateLoad(child_type, ref_loc);
}

llvm::Value *ArrayAccessExpresion::codegen(ContextHolder holder) {
  // the leaf would return the result
  if (m_child_posfix_expression)
    return m_child_posfix_expression->codegen(holder);

  // we are at the leaf
  llvm::Value *start_of_pointer = getRef(holder);
  if (m_compute_ref)
    return start_of_pointer;

  return holder->builder.CreateLoad(getChildType(holder)->getType(holder),
                                    start_of_pointer);
}
