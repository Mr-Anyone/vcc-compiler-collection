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

Statement::Statement(const std::vector<ASTBase *> childrens)
    : ASTBase(std::vector<Statement *>()) {
  for (ASTBase *child : childrens) {
    addChildren(child);
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

ASTBase::ASTBase(const std::vector<Expression *> childrens)
    : m_parent(nullptr), m_childrens() {

  for (ASTBase *children : childrens) {
    addChildren(children);
    children->setParent(this);
  }
}

ASTBase::ASTBase(const std::vector<Statement *> childrens)
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

FunctionDecl::FunctionDecl(std::vector<Statement *> &statements,
                           FunctionArgLists *arg_list, std::string &&name,
                           Type *ret, bool is_extern)
    : Statement({arg_list}), m_statements(statements), m_arg_list(arg_list),
      m_name(name), m_return_type(ret), m_is_extern(is_extern) {
  // making sure that arg_list is always the first in the syntax tree!
  for (ASTBase *statement : statements) {
    addChildren(statement);
  }
}

FunctionArgLists::FunctionArgLists(std::vector<TypeInfo> &&args)
    : Statement({}), m_args(args) {}

FunctionArgLists::ArgsIter FunctionArgLists::begin() const {
  return m_args.cbegin();
}

FunctionArgLists::ArgsIter FunctionArgLists::end() const {
  return m_args.cend();
}

AssignmentStatement::AssignmentStatement(ASTBase *ref_expr, ASTBase *expression)
    : Statement({ref_expr, expression}), m_ref_expr(ref_expr),
      m_expression(expression) {}

void FunctionDecl::dump() {
  std::cout << "name: " << m_name << " args: extern: " << m_is_extern;
  for (auto it = m_arg_list->begin(), ie = m_arg_list->end(); it != ie; ++it) {
    std::cout << it->name << ", ";
  }
}

ReturnStatement::ReturnStatement(Expression *expression)
    : Statement({expression}), m_expression(expression) {}

IdentifierExpr::IdentifierExpr(const std::string &name)
    : LocatorExpression({}), m_name(name) {}

ConstantExpr::ConstantExpr(int value) : Expression({}), m_value(value) {}

int ConstantExpr::getValue() { return m_value; }

BinaryExpression::BinaryExpression(Expression *lhs, BinaryExpressionType type)
    : Expression({lhs}), m_lhs(lhs), m_rhs(nullptr), m_kind(type) {}

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
  case lex::Divide:
    return Divide;
  default:
    assert(false && "invalid token type");
    return Add;
  }
}

void BinaryExpression::setRHS(Expression *rhs) {
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
                   const std::vector<Expression *> &expression)
    : Expression(expression), m_func_name(name), m_expressions(expression) {}

void CallExpr::dump() { std::cout << "name: " << m_func_name; }

IfStatement::IfStatement(Expression *cond,
                         std::vector<Statement *> &&expressions)
    : Statement({cond}), m_cond(cond), m_statements(expressions) {
  for (ASTBase *expression : expressions) {
    addChildren(expression);
  }
}

void IfStatement::dump() {}

DeclarationStatement::DeclarationStatement(const std::string &name,
                                           Expression *base, Type *type)
    : Statement({}), m_expression(base), m_name(name), m_type(type) {
  // it is possible that the child is a nullptr, meaning we only have to
  // allocate space
  if (base)
    addChildren(base);
}

void DeclarationStatement::dump() { std::cout << "name: " << m_name; }

WhileStatement::WhileStatement(Expression *cond,
                               std::vector<Statement *> &&expression)
    : Statement({cond}), m_cond(cond), m_statements(expression) {
  for (ASTBase *base : m_statements) {
    addChildren(base);
  }
}

void WhileStatement::dump() { return; }

MemberAccessExpression::MemberAccessExpression(const std::string &name,
                                               const std::string &member)
    : m_base_name(name), m_member(member), LocatorExpression({}) {}

MemberAccessExpression::MemberAccessExpression(LocatorExpression *parent,
                                               const std::string &member)
    : m_member(member), LocatorExpression({}), m_parent(parent) {
  parent->addChildren(this);
}

void MemberAccessExpression::dump() {
  // if we don't have a parent, we must have a valid m_base_name
  if (!m_parent)
    std::cout << m_base_name;

  std::cout << "." << m_member << " child: " << m_child_posfix_expression
            << " this: " << this;
}

ArrayAccessExpression::ArrayAccessExpression(const std::string &name,
                                           Expression *expression)
    : LocatorExpression({expression}), m_index_expression(expression),
      m_base_name(name) {}

ArrayAccessExpression::ArrayAccessExpression(LocatorExpression *parent,
                                           Expression *index_expression)
    : LocatorExpression({index_expression}),
      m_index_expression(index_expression), m_parent_expression(parent) {
  parent->addChildren(this);
}

void ArrayAccessExpression::dump() {
  std::cout << "[]"
            << " child*: " << m_child_posfix_expression << " this: " << this;
}

LocatorExpression::LocatorExpression(const std::vector<Expression *> &childrens)
    : Expression(childrens) {}

Type *ArrayAccessExpression::getGEPChildType(ContextHolder holder) {
  Type *current_type = getGEPType(holder);
  assert((current_type->isArray() || current_type->isPointer()) &&
         "array access expression must have valid type!");

  if (PointerType *type = dyncast<PointerType>(current_type)) {
    return type->getPointee();
  }

  return current_type->getAs<ArrayType>()->getBase();
}

Type *MemberAccessExpression::getGEPChildType(ContextHolder holder) {
  return getGEPType(holder)
      ->getAs<StructType>()
      ->getElement(m_member)
      .value()
      .type;
}

void ArrayAccessExpression::setChildPosfixExpression(LocatorExpression *child) {
  m_child_posfix_expression = child;
}

void MemberAccessExpression::setChildPosfixExpression(
    LocatorExpression *child) {
  m_child_posfix_expression = child;
}

llvm::Value *LocatorExpression::getRef(ContextHolder holder) {
  assert(false && "please implement this!'");
}
llvm::Value *IdentifierExpr::getRef(ContextHolder holder) {
  return holder->symbol_table.lookupLocalVariable(this, m_name).value;
}

static llvm::Value* getStartOfPointerFromParent(Expression* expression, ContextHolder holder){
    if(MemberAccessExpression* member = dyncast<MemberAccessExpression>(expression))
        return member->getCurrentRef(holder);

    return static_cast<ArrayAccessExpression*>(expression)->getCurrentRef(holder);
}

llvm::Value *MemberAccessExpression::getCurrentRef(ContextHolder holder) {
  llvm::Value *start_of_pointer =
      (m_parent == nullptr)
          ? holder->symbol_table.lookupLocalVariable(this, m_base_name).value
          : getStartOfPointerFromParent(m_parent, holder);

  // getting the actual field number
  Type *current_type = getGEPType(holder);
  int field_num =
      current_type->getAs<StructType>()->getElement(m_member)->field_num;
  llvm::Value *zero =
      llvm::ConstantInt::get(llvm::Type::getInt32Ty(holder->context), 0);
  llvm::Value *offset = llvm::ConstantInt::get(
      llvm::Type::getInt32Ty(holder->context), field_num);

  return holder->builder.CreateGEP(current_type->getType(holder),
                                   start_of_pointer, {zero, offset});
}

llvm::Value *ArrayAccessExpression::getCurrentRef(ContextHolder holder) {
  // if we don't have a parent, then we can get the
  // location from a symbol table lookup! It means we
  // are at the most top level! Or else we get the location from parent's getRef
  llvm::Value *start_of_pointer =
      (m_parent_expression == nullptr)
          ? holder->symbol_table.lookupLocalVariable(this, m_base_name).value
          : getStartOfPointerFromParent(m_parent_expression, holder);

  Type *type = getGEPType(holder);
  llvm::Type *llvm_type = getGEPType(holder)->getType(holder);
  // we cannot just do type->isPointer because the last layer
  // returns a builtin usually say i32**
  if (!type->isArray())
    start_of_pointer = holder->builder.CreateLoad(
        llvm::PointerType::get(holder->context, /*AddressSpace*/ 0),
        start_of_pointer);

  llvm::ConstantInt *zero =
      llvm::ConstantInt::get(llvm::Type::getInt32Ty(holder->context), 0);
  llvm::Value *offset = m_index_expression->getVal(holder);

  return holder->builder.CreateGEP(
      llvm_type, start_of_pointer,
      (type->isArray()) ? std::vector<llvm::Value *>{zero, offset}
                        : std::vector<llvm::Value *>{offset});
}

Type *FunctionDecl::getReturnType() const { return m_return_type; }

Expression::Expression(const std::vector<Expression *> children)
    : ASTBase(children) {}

DeRefExpression::DeRefExpression(Expression *ref_get)
    : LocatorExpression({ref_get}), m_ref(ref_get) {}

void DeRefExpression::dump() {}

// ================================================================================
// ====================== Expression Implementation::getType
// ======================
Type *MemberAccessExpression::getGEPType(ContextHolder holder) {
  if (!m_parent)
    return holder->symbol_table.lookupLocalVariable(this, m_base_name).type;

  // FIXME: this shares a lot same code with ArrayAccessExpression::getType,
  // maybe we should have a standard interface that solves this entirely?
  if (ArrayAccessExpression *parent = dyncast<ArrayAccessExpression>(m_parent)) {
    return parent->getGEPChildType(holder);
  }
  assert(isa<MemberAccessExpression>(m_parent));
  MemberAccessExpression *parent = dyncast<MemberAccessExpression>(m_parent);
  return parent->getGEPChildType(holder);
}

Type *MemberAccessExpression::getType(ContextHolder holder) {
  if (m_child_posfix_expression)
    return m_child_posfix_expression->getType(holder);

  // we don't have a child
  return getGEPType(holder)->getAs<StructType>()->getElement(m_member)->type;
}

Type *DeRefExpression::getType(ContextHolder holder) {
  return dyncast<Expression>(m_ref)
      ->getType(holder)
      ->getAs<PointerType>()
      ->getPointee();
}

Type *ArrayAccessExpression::getType(ContextHolder holder) {
  if (m_child_posfix_expression)
    return m_child_posfix_expression->getType(holder);

  return getGEPType(holder);
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

Type *BinaryExpression::getType(ContextHolder holder) {
  assert(false && "not sure what to do this");
}

// FIXME: this really should be callled getGEP type and not getType
// getType returns the result of the ast expression, but currently, this returns
// the GEP type
Type *ArrayAccessExpression::getGEPType(ContextHolder holder) {
  if (!m_parent_expression) {
    Type *type =
        holder->symbol_table.lookupLocalVariable(this, m_base_name).type;
    if (type->isArray())
      return type->getAs<ArrayType>();

    return type->getAs<PointerType>()->getPointee();
  }

  // trying to get type from parent expression!
  if (ArrayAccessExpression *parent =
          dyncast<ArrayAccessExpression>(m_parent_expression)) {
    return parent->getGEPChildType(holder);
  }

  assert(isa<MemberAccessExpression>(m_parent_expression) &&
         "must be member expresion beacuse we have no options left!");
  MemberAccessExpression *parent =
      dyncast<MemberAccessExpression>(m_parent_expression);
  return parent->getGEPChildType(holder);
}

// ======================================================
// ====================== CODE GEN ======================
llvm::Value *ConstantExpr::getVal(ContextHolder holder) {
  llvm::Value *value =
      llvm::ConstantInt::get(llvm::Type::getInt32Ty(holder->context), m_value);
  return value;
}

llvm::Value *BinaryExpression::handleInteger(ContextHolder holder,
                                             llvm::Value *right_hand_side,
                                             llvm::Value *left_hand_side) {
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
  case Divide: {
    assert(left_hand_side->getType()->isIntegerTy() &&
           right_hand_side->getType()->isIntegerTy());
    llvm::Value *check =
        holder->builder.CreateUDiv(left_hand_side, right_hand_side);
    return check;
  }
  default:
    assert(false && "cannot get here");
    return nullptr;
  }
}

llvm::Value *BinaryExpression::getVal(ContextHolder holder) {
  llvm::Value *right_hand_side = m_rhs->getVal(holder);
  llvm::Value *left_hand_side = m_lhs->getVal(holder);

  if (right_hand_side->getType()->isIntegerTy() &&
      left_hand_side->getType()->isIntegerTy())
    return handleInteger(holder, left_hand_side, right_hand_side);

  assert((right_hand_side->getType()->isFloatingPointTy() ||
          left_hand_side->getType()->isFloatingPointTy()) &&
         "either side must be a floating point");

  // Implicit conversion will take place between right hand side and left hand
  // side of a binary expression if the two given type are float and int.  In
  // such case, the implicit conversion will take place in int and be converted
  // into a float.
  if (!right_hand_side->getType()->isFloatingPointTy()) {
    right_hand_side = holder->builder.CreateSIToFP(right_hand_side,
                                                   left_hand_side->getType());
  } else if (!left_hand_side->getType()->isFloatingPointTy()) {
    assert(left_hand_side->getType()->isIntegerTy());
    left_hand_side = holder->builder.CreateSIToFP(left_hand_side,
                                                  right_hand_side->getType());
  }

  assert(right_hand_side && left_hand_side && "cannot be null");
  assert(left_hand_side->getType() == right_hand_side->getType());
  switch (m_kind) {
  case Add: {
    llvm::Value *result =
        holder->builder.CreateFAdd(left_hand_side, right_hand_side);
    return result;
  }
  case Multiply: {
    llvm::Value *reuslt =
        holder->builder.CreateFMul(left_hand_side, right_hand_side);
    return reuslt;
  }
  case Equal: {
    left_hand_side->getType();
    llvm::Value *equal_check =
        holder->builder.CreateFCmpOEQ(left_hand_side, right_hand_side);
    return equal_check;
  }
  case NEquals: {
    llvm::Value *not_equal_check =
        holder->builder.CreateFCmpONE(left_hand_side, right_hand_side);
    return not_equal_check;
  }
  case GE: {
    llvm::Value *check =
        holder->builder.CreateFCmpOGE(left_hand_side, right_hand_side);
    return check;
  }
  case GT: {
    llvm::Value *check =
        holder->builder.CreateFCmpOGT(left_hand_side, right_hand_side);
    return check;
  }
  case Subtract: {
    llvm::Value *subtract =
        holder->builder.CreateFSub(left_hand_side, right_hand_side);
    return subtract;
  }
  case LE: {
    llvm::Value *check =
        holder->builder.CreateFCmpOLE(left_hand_side, right_hand_side);
    return check;
  }
  case LT: {
    llvm::Value *check =
        holder->builder.CreateFCmpOLT(left_hand_side, right_hand_side);
    return check;
  }
  case Divide: {
    assert(left_hand_side->getType()->isIntegerTy() &&
           right_hand_side->getType()->isIntegerTy());
    llvm::Value *check =
        holder->builder.CreateFDiv(left_hand_side, right_hand_side);
    return check;
  }
  default:
    assert(false && "cannot get here");
    return nullptr;
  }

  assert(false && "how did we get here");
  return nullptr;
}

llvm::Value *IdentifierExpr::getVal(ContextHolder holder) {
  llvm::Value *loc_value =
      holder->symbol_table.lookupLocalVariable(this, m_name).value;

  llvm::Value *value =
      holder->builder.CreateLoad(getType(holder)->getType(holder), loc_value);
  return value;
}

void FunctionArgLists::codegen(ContextHolder holder) {
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
}

void AssignmentStatement::codegen(ContextHolder holder) {
  assert(false && "this is not yet done");
  // llvm::Value *expression_val = m_expression->codegen(holder);
  // llvm::Value *alloc_loc = m_ref_expr->codegen(holder);

  // assert(expression_val && alloc_loc);
  // holder->builder.CreateStore(expression_val, alloc_loc);
}

void AssignmentStatement::dump() {}

void ReturnStatement::codegen(ContextHolder holder) {
  // FIXME: must add semantics analysis
  llvm::Value *return_value = m_expression->getVal(holder);

  holder->builder.CreateRet(return_value);
}

void FunctionDecl::buildExternalDecl(ContextHolder holder) {
  llvm::FunctionType *function_type = getFunctionType(holder);
  holder->symbol_table.addFunction(this);
  m_function = llvm::Function::Create(
      function_type, llvm::Function::ExternalLinkage, m_name, holder->module);
}

llvm::FunctionType *FunctionDecl::getFunctionType(ContextHolder holder) const {
  std::vector<llvm::Type *> args;
  for (auto it = m_arg_list->begin(), ie = m_arg_list->end(); it != ie; ++it) {
    args.push_back(it->type->getType(holder));
  }

  llvm::FunctionType *function_type = llvm::FunctionType::get(
      m_return_type->getType(holder), args, /*isVarArg=*/false);
  return function_type;
}

void FunctionDecl::codegen(ContextHolder holder) {
  if (m_is_extern)
    return buildExternalDecl(holder);
  llvm::FunctionType *function_type = getFunctionType(holder);

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
  for (Statement *statement : m_statements) {
    statement->codegen(holder);
  }
}

llvm::Value *CallExpr::getVal(ContextHolder holder) {
  const FunctionDecl *function_decl =
      holder->symbol_table.lookupFunction(m_func_name);

  assert(function_decl && "this must exist for codegen!");
  assert(function_decl->getFunctionType(holder)->getNumParams() ==
             m_expressions.size() &&
         "expected the same number of argument");

  std::vector<llvm::Value *> args;
  for (Expression *expression : m_expressions) {
    args.push_back(expression->getVal(holder));
  }

  llvm::Value *result =
      holder->builder.CreateCall(function_decl->getFunctionType(holder),
                                 function_decl->getLLVMFunction(), args);

  return result;
}

void IfStatement::codegen(ContextHolder holder) {
  llvm::Function *function = getFirstFunctionDecl()->getLLVMFunction();
  llvm::BasicBlock *true_if_block =
      llvm::BasicBlock::Create(holder->context, "", function);
  llvm::BasicBlock *fallthrough_block =
      llvm::BasicBlock::Create(holder->context, "", function);

  llvm::Value *cond = m_cond->getVal(holder);
  assert(cond->getType()->isIntegerTy() && "must be integer type");
  cond = holder->builder.CreateICmpNE(
      cond, llvm::ConstantInt::get(cond->getType(), 0));

  llvm::Value *stuff =
      holder->builder.CreateCondBr(cond, true_if_block, fallthrough_block);

  holder->builder.SetInsertPoint(true_if_block);
  for (Statement *expression : m_statements) {
    expression->codegen(holder);
  }

  assert(m_statements.size() >= 1 && "must be true for now");
  ASTBase *last_expression = m_statements[m_statements.size() - 1];
  if (dynamic_cast<ReturnStatement *>(last_expression) == nullptr)
    holder->builder.CreateBr(fallthrough_block);

  holder->builder.SetInsertPoint(fallthrough_block);
}

void DeclarationStatement::codegen(ContextHolder holder) {
  // initialize the first variable
  const FunctionDecl *func = getFirstFunctionDecl();
  llvm::Value *alloc_loc =
      holder->builder.CreateAlloca(m_type->getType(holder));
  holder->symbol_table.addLocalVariable(this, m_name, m_type, alloc_loc);

  // if we don't have an initializer, we don't allocate space
  if (m_expression) {
    llvm::Value *exp = m_expression->getVal(holder);
    llvm::Value *return_val = holder->builder.CreateStore(exp, alloc_loc);
  }
}

void WhileStatement::codegen(ContextHolder holder) {
  llvm::BasicBlock *cond_block = llvm::BasicBlock::Create(
      holder->context, "", getFirstFunctionDecl()->getLLVMFunction());
  llvm::BasicBlock *while_true_block = llvm::BasicBlock::Create(
      holder->context, "", getFirstFunctionDecl()->getLLVMFunction());
  llvm::BasicBlock *fallthrough = llvm::BasicBlock::Create(
      holder->context, "", getFirstFunctionDecl()->getLLVMFunction());

  holder->builder.CreateBr(cond_block);

  // set up the cond block
  holder->builder.SetInsertPoint(cond_block);
  llvm::Value *cond = m_cond->getVal(holder);
  assert(cond->getType()->isIntegerTy() && "must be integer");
  cond = holder->builder.CreateICmpNE(
      cond, llvm::ConstantInt::get(cond->getType(), 0));
  holder->builder.CreateCondBr(cond, while_true_block, fallthrough);

  // set up while body block
  holder->builder.SetInsertPoint(while_true_block);
  for (Statement *statement : m_statements) {
    statement->codegen(holder);
  }
  assert(m_statements.size() >= 1 && "must be true for now");
  Statement *last_statement = m_statements[m_statements.size() - 1];
  if (!isa<ReturnStatement>(last_statement))
    holder->builder.CreateBr(cond_block);

  holder->builder.SetInsertPoint(fallthrough);
}

llvm::Value *MemberAccessExpression::getVal(ContextHolder holder) {
  assert(false && "this is currently wrong");
  if (m_child_posfix_expression)
    return m_child_posfix_expression->getVal(holder);

  // we are at the base case
  llvm::Value *ref_loc = getRef(holder);
  llvm::Type *child_type = getGEPType(holder)
                               ->getAs<StructType>()
                               ->getElement(m_member)
                               ->type->getType(holder);
  return holder->builder.CreateLoad(child_type, ref_loc);
}

llvm::Value *ArrayAccessExpression::getVal(ContextHolder holder) {
  assert(false && "fix me later");
  // the leaf would return the result
  if (m_child_posfix_expression)
    return m_child_posfix_expression->getVal(holder);

  // we are at the leaf
  llvm::Value *start_of_pointer = getRef(holder);
  Type *current_type = getGEPType(
      holder); // it is possible that this is i32 already, so we just load it
  Type *child_type =
      current_type->isBuiltin() ? current_type : getGEPChildType(holder);
  return holder->builder.CreateLoad(child_type->getType(holder),
                                    start_of_pointer);
}

llvm::Value *DeRefExpression::getVal(ContextHolder holder) {
  assert(m_ref->getType(holder)->isPointer());
  llvm::Value *current_value = m_ref->getVal(holder);
  llvm::Type *base_type =
      m_ref->getType(holder)->getAs<PointerType>()->getPointee()->getType(
          holder);

  return holder->builder.CreateLoad(base_type, current_value);
}

llvm::Value *DeRefExpression::getRef(ContextHolder holder) { 
    assert(m_ref->getType(holder)->isPointer());
    return m_ref->getVal(holder);
}
