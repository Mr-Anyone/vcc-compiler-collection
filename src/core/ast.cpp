#include "core/ast.h"
#include "core/type.h"
#include "core/util.h"

#include <cassert>
#include <iostream>
#include <llvm/IR/Constant.h>
#include <llvm/IR/DerivedTypes.h>

using namespace vcc;

static void printSpaceBasedOnDepth(int depth) {
  for (int i = 0; i < depth * 2 - 1; ++i) {
    std::cout << " ";
  }
}

const FilePos &ASTBase::getPos() const { return m_locus; }

Statement::Statement(const std::vector<ASTBase *> childrens, FilePos locus)
    : ASTBase(std::vector<Statement *>(), locus) {
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

ASTBase::ASTBase(const std::vector<Expression *> childrens, FilePos locus)
    : m_parent(nullptr), m_childrens(), m_locus(locus) {

  for (ASTBase *children : childrens) {
    addChildren(children);
    children->setParent(this);
  }
}

ASTBase::ASTBase(const std::vector<Statement *> childrens, FilePos locus)
    : m_parent(nullptr), m_childrens(), m_locus(locus) {

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

void AssignmentStatement::dump() {}

const ASTBase *ASTBase::getParent() const { return m_parent; }

void ASTBase::dump() { return; }

const std::string &FunctionDecl::getName() const { return m_name; }

llvm::Function *FunctionDecl::getLLVMFunction() const { return m_function; }

FunctionDecl::FunctionDecl(std::vector<Statement *> &statements,
                           FunctionArgLists *arg_list, std::string &&name,
                           Type *ret, bool is_extern, FilePos locus)
    : Statement({arg_list}, locus), m_statements(statements),
      m_arg_list(arg_list), m_name(name), m_return_type(ret),
      m_is_extern(is_extern) {
  // making sure that arg_list is always the first in the syntax tree!
  for (ASTBase *statement : statements) {
    addChildren(statement);
  }
}

const FunctionArgLists::ArgsIter FunctionDecl::getArgBegin() const {
  return m_arg_list->begin();
}

const FunctionArgLists::ArgsIter FunctionDecl::getArgsEnd() const {
  return m_arg_list->end();
}

FunctionArgLists::FunctionArgLists(std::vector<TypeInfo> &&args, FilePos locus)
    : Statement({}, locus), m_args(args) {}

FunctionArgLists::ArgsIter FunctionArgLists::begin() const {
  return m_args.cbegin();
}

FunctionArgLists::ArgsIter FunctionArgLists::end() const {
  return m_args.cend();
}

AssignmentStatement::AssignmentStatement(Expression *ref_expr,
                                         Expression *expression, FilePos locus)
    : Statement({ref_expr, expression}, locus), m_ref_expr(ref_expr),
      m_expression(expression) {}

void FunctionDecl::dump() {
  std::cout << "name: " << m_name << " args: extern: " << m_is_extern;
  for (auto it = m_arg_list->begin(), ie = m_arg_list->end(); it != ie; ++it) {
    std::cout << it->name << ", ";
  }
}

ReturnStatement::ReturnStatement(Expression *expression, FilePos locus)
    : Statement({}, locus), m_expression(expression) {
  // it is possible that expression is null
  if (expression)
    addChildren(expression);
}

IdentifierExpr::IdentifierExpr(const std::string &name, FilePos locus)
    : LocatorExpression({}, locus), m_name(name) {}

ConstantExpr::ConstantExpr(int value, FilePos locus)
    : Expression({}, locus), m_value(value) {}

int ConstantExpr::getValue() { return m_value; }

BinaryExpression::BinaryExpression(Expression *lhs, BinaryExpressionType type,
                                   FilePos locus)
    : Expression({lhs}, locus), m_lhs(lhs), m_rhs(nullptr), m_kind(type) {}

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
  return doesDefineScope(at->getCode());
}

bool ASTBase::doesDefineScope(code::TreeCode code) {
  return code == code::FunctionDecl || code == code::IfStatement ||
         code == code::WhileStatement;
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
                   const std::vector<Expression *> &expression, FilePos locus)
    : Expression(expression, locus), m_func_name(name),
      m_expressions(expression) {}

void CallExpr::dump() { std::cout << "name: " << m_func_name; }

IfStatement::IfStatement(Expression *cond,
                         std::vector<Statement *> &&expressions, FilePos locus)
    : Statement({cond}, locus), m_cond(cond), m_statements(expressions) {
  for (ASTBase *expression : expressions) {
    addChildren(expression);
  }
}

void IfStatement::dump() {}

DeclarationStatement::DeclarationStatement(const std::string &name,
                                           Expression *base, Type *type,
                                           FilePos locus)
    : Statement({}, locus), m_expression(base), m_name(name), m_type(type) {
  // it is possible that the child is a nullptr, meaning we only have to
  // allocate space
  if (base)
    addChildren(base);
}

const std::string &DeclarationStatement::getName() { return m_name; }

Type *DeclarationStatement::getType() { return m_type; }

void DeclarationStatement::dump() { std::cout << "name: " << m_name; }

WhileStatement::WhileStatement(Expression *cond,
                               std::vector<Statement *> &&expression,
                               FilePos locus)
    : Statement({cond}, locus), m_cond(cond), m_statements(expression) {
  for (ASTBase *base : m_statements) {
    addChildren(base);
  }
}

void WhileStatement::dump() { return; }

MemberAccessExpression::MemberAccessExpression(const std::string &name,
                                               const std::string &member,
                                               FilePos locus)
    : m_base_name(name), m_member(member), LocatorExpression({}, locus) {}

MemberAccessExpression::MemberAccessExpression(LocatorExpression *parent,
                                               const std::string &member,
                                               FilePos locus)
    : m_member(member), LocatorExpression({}, locus), m_parent(parent) {
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
                                             Expression *expression,
                                             FilePos locus)
    : LocatorExpression({expression}, locus), m_index_expression(expression),
      m_base_name(name) {}

ArrayAccessExpression::ArrayAccessExpression(LocatorExpression *parent,
                                             Expression *index_expression,
                                             FilePos locus)
    : LocatorExpression({index_expression}, locus),
      m_index_expression(index_expression), m_parent_expression(parent) {
  parent->addChildren(this);
}

void ArrayAccessExpression::dump() {
  std::cout << "[]"
            << " child*: " << m_child_posfix_expression << " this: " << this;
}

LocatorExpression::LocatorExpression(const std::vector<Expression *> &childrens,
                                     FilePos locus)
    : Expression(childrens, locus) {}

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

void DeRefExpression::setPosfixChildExpression(LocatorExpression *expression) {
  m_posfix_child = expression;
}

void ArrayAccessExpression::setChildPosfixExpression(LocatorExpression *child) {
  m_child_posfix_expression = child;
}

void MemberAccessExpression::setChildPosfixExpression(
    LocatorExpression *child) {
  m_child_posfix_expression = child;
}

llvm::Value *ArrayAccessExpression::getRef(ContextHolder holder) {
  // We will be call getCurrentRef on the base to get the reference on the
  // entire class
  if (m_child_posfix_expression)
    return m_child_posfix_expression->getRef(holder);

  return getCurrentRef(holder);
}

llvm::Value *MemberAccessExpression::getRef(ContextHolder holder) {
  // We will be call getCurrentRef on the base to get the reference on the
  // entire class
  if (m_child_posfix_expression)
    return m_child_posfix_expression->getRef(holder);

  return getCurrentRef(holder);
}

llvm::Value *LocatorExpression::getRef(ContextHolder holder) {
  assert(false && "please implement this!'");
  return nullptr;
}

llvm::Value *IdentifierExpr::getRef(ContextHolder holder) {
  return holder->symbol_table.lookupLocalVariable(this, m_name).value;
}

static llvm::Value *getStartOfPointerFromParent(Expression *expression,
                                                ContextHolder holder) {
  if (MemberAccessExpression *member =
          dyncast<MemberAccessExpression>(expression))
    return member->getCurrentRef(holder);

  if (DeRefExpression *ref = dyncast<DeRefExpression>(expression)) {
    return ref->getCurrentRef(holder);
  }

  return dyncast<ArrayAccessExpression>(expression)->getCurrentRef(holder);
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

Expression::Expression(const std::vector<Expression *> children, FilePos locus)
    : ASTBase(children, locus) {}

DeRefExpression::DeRefExpression(Expression *ref_get, FilePos locus)
    : LocatorExpression({ref_get}, locus), m_ref(ref_get) {}

void DeRefExpression::dump() {}

RefExpression::RefExpression(Expression *inner, FilePos locus)
    : LocatorExpression({inner}, locus), m_inner_expression(inner) {}

void RefExpression::dump() {}

llvm::FunctionType *FunctionDecl::getFunctionType(ContextHolder holder) const {
  std::vector<llvm::Type *> args;
  for (auto it = m_arg_list->begin(), ie = m_arg_list->end(); it != ie; ++it) {
    args.push_back(it->type->getType(holder));
  }

  llvm::FunctionType *function_type = llvm::FunctionType::get(
      m_return_type->getType(holder), args, /*isVarArg=*/false);
  return function_type;
}

CallStatement::CallStatement(Expression *call_expression, FilePos locus)
    : Statement({call_expression}, locus), m_call_expr(call_expression) {}

StringLiteral::StringLiteral(std::string string, FilePos locus)
    : Expression({}, locus), m_string_literal(string) {}

void StringLiteral::dump() {}

CastExpression::CastExpression(Expression *cast_expression, Type *casted_to,
                               FilePos loc)
    : Expression({cast_expression}, loc), m_cast_to(casted_to),
      m_to_be_casted_expression(cast_expression) {}

void CastExpression::emitErrorAndExit(ContextHolder holder) {
  // we cannot perform a cast emit a diagnostics message
  holder->diagnostics.diag(this, holder->getLine(getPos()),
                           "cannot perform a cast");
  std::exit(-1);
}

static std::vector<DeclarationStatement *>
getDeclarationStatementImpl(const std::vector<Statement *> &statement) {
  std::vector<DeclarationStatement *> result;
  for (Statement *s : statement) {
    if (ASTBase::doesDefineScope(s->getCode())) {
      switch (s->getCode()) {
      case vcc::code::WhileStatement: {
        auto other = dyncast<WhileStatement>(s)->getDeclarationStatements();
        result.insert(result.begin(), other.begin(), other.end());
        break;
      }
      case vcc::code::IfStatement: {
        auto other = dyncast<IfStatement>(s)->getDeclarationStatements();
        result.insert(result.begin(), other.begin(), other.end());
        break;
      }
      default:
        std::cerr << "you have missed a case" << std::endl;
        std::exit(-1);
      }
    }
    if (s->getCode() == code::DeclarationStatement)
      result.push_back(dyncast<DeclarationStatement>(s));
  }

  return result;
}

std::vector<DeclarationStatement *>
WhileStatement::getDeclarationStatements() const {
  return getDeclarationStatementImpl(m_statements);
}

std::vector<DeclarationStatement *>
IfStatement::getDeclarationStatements() const {
  return getDeclarationStatementImpl(m_statements);
}

Expression *DeclarationStatement::getExpression() { return m_expression; }

// ================================================================================
// ====================== Expression Implementation::getType
// ======================
Type *CastExpression::getType(ContextHolder holder) { return m_cast_to; }

Type *StringLiteral::getType(ContextHolder holder) {
  return new PointerType(new BuiltinType(BuiltinType::Char));
}

Type *RefExpression::getType(ContextHolder holder) {
  // FIXME: we can probably prevent a heap allocation every time
  return new PointerType(m_inner_expression->getType(holder));
}

Type *MemberAccessExpression::getGEPType(ContextHolder holder) {
  if (!m_parent)
    return holder->symbol_table.lookupLocalVariable(this, m_base_name).type;

  // FIXME: this shares a lot same code with ArrayAccessExpression::getType,
  // maybe we should have a standard interface that solves this entirely?
  if (ArrayAccessExpression *parent =
          dyncast<ArrayAccessExpression>(m_parent)) {
    return parent->getGEPChildType(holder);
  }

  if (DeRefExpression *expression = dyncast<DeRefExpression>(m_parent)) {
    return expression->getInnerType(holder);
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
  if (m_posfix_child)
    return m_posfix_child->getType(holder);

  return dyncast<Expression>(m_ref)
      ->getType(holder)
      ->getAs<PointerType>()
      ->getPointee();
}

Type *DeRefExpression::getInnerType(ContextHolder holder) {
  return dyncast<Expression>(m_ref)
      ->getType(holder)
      ->getAs<PointerType>()
      ->getPointee();
}

Type *ArrayAccessExpression::getType(ContextHolder holder) {
  if (m_child_posfix_expression)
    return m_child_posfix_expression->getType(holder);

  if (getGEPType(holder)->isBuiltin()) {
    return getGEPType(holder);
  }
  return getGEPChildType(holder);
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

static Type *getIntWithMoreBits(BuiltinType *lhs, BuiltinType *rhs) {
  assert(lhs->isIntegerKind() && rhs->isIntegerKind());
  return lhs->getBitSize() > rhs->getBitSize() ? lhs : rhs;
}

Type *BinaryExpression::getType(ContextHolder holder) {
  // check for boolean expression
  switch (m_kind) {
  // if it is from a boolean expression, it should always return a boolean
  // expression regardless of the two types
  case Equal:
  case NEquals:
  case GE:
  case GT:
  case LE:
  case LT:
    return new BuiltinType(BuiltinType::Bool);
  default:
    break;
  };

  if (m_lhs->getType(holder)->isPointer() ||
      m_rhs->getType(holder)->isPointer()) {
    assert(false && "please emit error here. pointer type in binary expression "
                    "is illform for now!");
    return nullptr;
  }

  if (m_lhs->getType(holder)->isBuiltin() &&
      m_rhs->getType(holder)->isBuiltin()) {
    BuiltinType *casted_lhs = m_lhs->getType(holder)->getAs<BuiltinType>();
    BuiltinType *casted_rhs = m_rhs->getType(holder)->getAs<BuiltinType>();

    // return float type if either the left hand side or the right hand side
    // have a floating point
    if (casted_lhs->isFloat())
      return casted_lhs;
    else if (casted_rhs->isFloat())
      return casted_rhs;

    return getIntWithMoreBits(casted_lhs, casted_rhs);
  }

  assert(false && "don't know what to do here!");
  return nullptr;
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

// ==========================================
// TreeCode Implementation
//
code::TreeCode FunctionArgLists::getCode() const {
  return code::FunctionArgLists;
}

code::TreeCode CallStatement::getCode() const { return code::CallStatement; }

code::TreeCode FunctionDecl::getCode() const { return code::FunctionDecl; }

code::TreeCode AssignmentStatement::getCode() const {
  return code::AssignmentStatement;
}

code::TreeCode ReturnStatement::getCode() const {
  return code::ReturnStatement;
}

code::TreeCode IfStatement::getCode() const { return code::IfStatement; }

code::TreeCode WhileStatement::getCode() const { return code::WhileStatement; }

code::TreeCode ConstantExpr::getCode() const { return code::ConstantExpr; }

code::TreeCode CallExpr::getCode() const { return code::CallExpr; }

code::TreeCode BinaryExpression::getCode() const {
  return code::BinaryExpression;
}

code::TreeCode CastExpression::getCode() const { return code::CastExpression; }

code::TreeCode IdentifierExpr::getCode() const { return code::IdentifierExpr; }

code::TreeCode MemberAccessExpression::getCode() const {
  return code::MemberAccessExpression;
}

code::TreeCode ArrayAccessExpression::getCode() const {
  return code::ArrayAccessExpression;
}

code::TreeCode DeRefExpression::getCode() const {
  return code::DeRefExpression;
}

code::TreeCode RefExpression::getCode() const { return code::RefExpression; }

code::TreeCode StringLiteral::getCode() const { return code::StringLiteral; }

code::TreeCode DeclarationStatement::getCode() const {
  return code::DeclarationStatement;
}

// ======================================================
// ====================== CODE GEN ======================
llvm::Value *RefExpression::getVal(ContextHolder holder) {
  return dyncast<LocatorExpression>(m_inner_expression)->getRef(holder);
}

llvm::Value *RefExpression::getRef(ContextHolder holder) {
  assert(false && "this is ill form");
  return dyncast<LocatorExpression>(m_inner_expression)->getRef(holder);
}

llvm::Value *ConstantExpr::getVal(ContextHolder holder) {
  llvm::Value *value =
      llvm::ConstantInt::get(llvm::Type::getInt32Ty(holder->context), m_value);
  return value;
}

llvm::Value *BinaryExpression::handleInteger(ContextHolder holder,
                                             llvm::Value *left_hand_side,
                                             llvm::Value *right_hand_side) {
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
  assert(isa<LocatorExpression>(m_ref_expr) && "must be an locator value");
  llvm::Value *expression_val = m_expression->getVal(holder);
  llvm::Value *alloc_loc =
      dyncast<LocatorExpression>(m_ref_expr)->getRef(holder);

  if (!Type::isSame(m_expression->getType(holder),
                    m_ref_expr->getType(holder))) {
    holder->diagnostics.diag(this, holder->getLine(getPos()), "invalid type");
    std::exit(-1);
    return;
  }

  assert(expression_val && alloc_loc);
  holder->builder.CreateStore(expression_val, alloc_loc);
}

void ReturnStatement::codegen(ContextHolder holder) {
  // FIXME: must add semantics analysis
  if (m_expression) {
    llvm::Value *return_value = m_expression->getVal(holder);
    holder->builder.CreateRet(return_value);

  } else {
    assert(getFirstFunctionDecl()->getReturnType()->isVoid() &&
           "must be void for this to make sense");
    holder->builder.CreateRetVoid();
  }
}

void FunctionDecl::buildExternalDecl(ContextHolder holder) {
  llvm::FunctionType *function_type = getFunctionType(holder);
  holder->symbol_table.addFunction(this);
  m_function = llvm::Function::Create(
      function_type, llvm::Function::ExternalLinkage, m_name, holder->module);
}

void CallStatement::codegen(ContextHolder holder) {
  m_call_expr->getVal(holder);
}

void FunctionDecl::emitAllocs(ContextHolder holder) {
  std::vector<DeclarationStatement *> declaration_statements{};

  // recursively getting all the declaration statements
  // and allocating the space at the beginning of FunctionDecl
  for (Statement *statement : m_statements) {
    switch (statement->getCode()) {
    case code::DeclarationStatement: {
      declaration_statements.push_back(
          dyncast<DeclarationStatement>(statement));
      break;
    }
    case code::WhileStatement: {
      std::vector<DeclarationStatement *> added =
          dyncast<WhileStatement>(statement)->getDeclarationStatements();
      declaration_statements.insert(declaration_statements.end(), added.begin(),
                                    added.end());
      break;
    }
    case code::IfStatement: {
      std::vector<DeclarationStatement *> added =
          dyncast<IfStatement>(statement)->getDeclarationStatements();
      declaration_statements.insert(declaration_statements.end(), added.begin(),
                                    added.end());
    }
    default:
      break;
    }
  }

  // allocating the space and inserting into trie tree
  for (DeclarationStatement *statement : declaration_statements) {
    llvm::Type *llvm_type = statement->getType()->getType(holder);
    llvm::Value *loc = holder->builder.CreateAlloca(llvm_type);
    holder->symbol_table.addLocalVariable(statement, statement->getName(),
                                          statement->getType(), loc);
  }
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
  emitAllocs(holder); // creating the space needed for declaration statement

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
  int count = 0;
  for (auto it = function_decl->getArgBegin(), ie = function_decl->getArgsEnd();
       it != ie; ++it) {
    if (!Type::isSame(it->type, m_expressions[count]->getType(holder))) {
      holder->diagnostics.diag(this, holder->getLine(getPos()),
                               "type mismatch");
      std::exit(-1);
    }

    args.push_back(m_expressions[count]->getVal(holder));
    ++count;
  }

  if (count != m_expressions.size()) {
    holder->diagnostics.diag(this, holder->getLine(getPos()),
                             "number of argument mismatch");
    std::exit(-1);
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
  for (Statement *statement : m_statements) {
    statement->codegen(holder);
  }

  assert(m_statements.size() >= 1 && "must be true for now");
  ASTBase *last_expression = m_statements[m_statements.size() - 1];
  if (dynamic_cast<ReturnStatement *>(last_expression) == nullptr)
    holder->builder.CreateBr(fallthrough_block);

  holder->builder.SetInsertPoint(fallthrough_block);
}

void DeclarationStatement::codegen(ContextHolder holder) {
  llvm::Value *alloc_loc =
      holder->symbol_table.lookupLocalVariable(this, m_name).value;

  // if we don't have an initializer, we don't allocate space
  if (m_expression) {
    if (!Type::isSame(m_type, m_expression->getType(holder))) {
      holder->diagnostics.diag(this, holder->getLine(getPos()),
                               "type mismatch");
      std::exit(-1);
      return;
    }
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
  if (m_child_posfix_expression)
    return m_child_posfix_expression->getVal(holder);

  // we are at the base case
  llvm::Value *ref_loc = getCurrentRef(holder);
  llvm::Type *child_type = getGEPType(holder)
                               ->getAs<StructType>()
                               ->getElement(m_member)
                               ->type->getType(holder);
  return holder->builder.CreateLoad(child_type, ref_loc);
}

llvm::Value *ArrayAccessExpression::getVal(ContextHolder holder) {
  // the leaf would return the result
  if (m_child_posfix_expression)
    return m_child_posfix_expression->getVal(holder);

  // we are at the leaf
  // FIXME: is this even correct?
  llvm::Value *start_of_pointer = getCurrentRef(holder);
  Type *current_type = getGEPType(holder);
  Type *child_type =
      current_type->isBuiltin() ? current_type : getGEPChildType(holder);
  return holder->builder.CreateLoad(child_type->getType(holder),
                                    start_of_pointer);
}

llvm::Value *DeRefExpression::getVal(ContextHolder holder) {
  // if we have a  posfix child, we recurse based on posfix child
  if (m_posfix_child)
    return m_posfix_child->getVal(holder);

  assert(m_ref->getType(holder)->isPointer());
  llvm::Value *current_value = m_ref->getVal(holder);
  llvm::Type *base_type =
      m_ref->getType(holder)->getAs<PointerType>()->getPointee()->getType(
          holder);

  return holder->builder.CreateLoad(base_type, current_value);
}

llvm::Value *DeRefExpression::getCurrentRef(ContextHolder holder) {
  assert(m_ref->getType(holder)->isPointer());
  return m_ref->getVal(holder);
}

llvm::Value *DeRefExpression::getRef(ContextHolder holder) {
  if (m_posfix_child)
    return m_posfix_child->getRef(holder);

  assert(m_ref->getType(holder)->isPointer());
  return m_ref->getVal(holder);
}

llvm::Value *StringLiteral::getVal(ContextHolder holder) {
  llvm::Value *global_string =
      holder->builder.CreateGlobalString(m_string_literal);

  return global_string;
}

llvm::Value *CastExpression::builtinCast(BuiltinType *from, BuiltinType *to,
                                         ContextHolder holder) {
  assert(from && to);
  assert(!Type::isSame(from, to));

  // handle int -> float
  if (from->isIntegerKind() && to->isFloat()) {
    llvm::Value *value = m_to_be_casted_expression->getVal(holder);
    return holder->builder.CreateSIToFP(value, to->getType(holder));
  }

  // handle float -> integer kind
  if (from->isFloat() && to->isIntegerKind()) {
    llvm::Value *value = m_to_be_casted_expression->getVal(holder);
    return holder->builder.CreateFPToSI(value, to->getType(holder));
  }

  // ==========================
  // handle integer  -> int in the following cases
  assert(from->isIntegerKind() && to->isIntegerKind());
  assert(from->getBitSize() != to->getBitSize());

  // we have a special case for bool where we zero extend instead of sign extend
  if (from->isBool()) {
    llvm::Value *val = m_to_be_casted_expression->getVal(holder);
    return holder->builder.CreateZExt(val, to->getType(holder));
  }

  if (from->getBitSize() > to->getBitSize()) {
    llvm::Value *val = m_to_be_casted_expression->getVal(holder);
    return holder->builder.CreateTrunc(val, to->getType(holder));
  } else {
    llvm::Value *val = m_to_be_casted_expression->getVal(holder);
    return holder->builder.CreateSExt(val, to->getType(holder));
  }
}

llvm::Value *CastExpression::getVal(ContextHolder holder) {
  Type *from_type = m_to_be_casted_expression->getType(holder);
  // we don't do anything if they are the same type
  if (Type::isSame(from_type, m_cast_to))
    return m_to_be_casted_expression->getVal(holder);

  if (from_type->isBuiltin() && m_cast_to->isBuiltin())
    return builtinCast(from_type->getAs<BuiltinType>(),
                       m_cast_to->getAs<BuiltinType>(), holder);

  // void pointer cast
  // this is okay since opaque pointer is already assumed in every pointer type
  if ((from_type->isPointer() && m_cast_to->isVoidPtr()) ||
      (from_type->isVoidPtr() && m_cast_to->isPointer()))
    return m_to_be_casted_expression->getVal(holder);

  emitErrorAndExit(holder);
  return nullptr;
}
