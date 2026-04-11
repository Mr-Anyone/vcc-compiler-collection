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
