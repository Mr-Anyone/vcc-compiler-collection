#include "core/ast.h"

#include <llvm/IR/Constant.h>
#include <llvm/IR/DerivedTypes.h>

#include <iostream>

#include "core/ast_dumper.h"
#include "core/type.h"
#include "core/util.h"

using namespace vcc;

const FilePos& ASTBase::getPos() const
{
    return m_locus;
}

Statement::Statement(code::TreeCode code, const std::vector<ASTBase*> childrens,
                     FilePos locus, Scope* scope)
    : ASTBase(code, std::vector<Statement*>(), locus, scope)
{
    for (ASTBase* child : childrens)
    {
        addChildren(child);
    }
}

Scope* ASTBase::getScope()
{
    return m_scope;
}

void ASTBase::debugDump(int depth)
{
    ASTDumper dumper;
    dumper.debugDump(this, std::cout, depth);
}

ASTBase::ASTBase(code::TreeCode code, const std::vector<Expression*> childrens,
                 FilePos locus, Scope* scope)
    : m_code(code), m_parent(nullptr), m_childrens(), m_locus(locus), m_scope(scope)
{
    for (ASTBase* children : childrens)
    {
        addChildren(children);
        children->setParent(this);
    }
}

ASTBase::ASTBase(code::TreeCode code, const std::vector<Statement*> childrens,
                 FilePos locus, Scope* scope)
    : m_code(code), m_parent(nullptr), m_childrens(), m_locus(locus), m_scope(scope)
{
    for (ASTBase* children : childrens)
    {
        addChildren(children);
        children->setParent(this);
    }
}

const std::set<ASTBase*>& ASTBase::getChildren() const
{
    return m_childrens;
}

void ASTBase::removeChildren(ASTBase* children)
{
    VCC_ASSERT(m_childrens.find(children) != m_childrens.end() &&
               "must contain element to begin with");

    // FIXME: we may have just leaked memory here!
    children->setParent(nullptr);
    m_childrens.erase(children);
}

void ASTBase::addChildren(ASTBase* children)
{
    m_childrens.insert(children);


    children->m_parent = this;
}

void ASTBase::setParent(ASTBase* parent)
{
    m_parent = parent;
    m_parent->m_childrens.insert(this);
}

const ASTBase* ASTBase::getParent() const
{
    return m_parent;
}

const std::string& FunctionDecl::getName() const
{
    return m_name;
}

FunctionDecl::FunctionDecl(std::vector<Statement*>& statements,
                           FunctionArgLists* arg_list, std::string&& name, Type* ret,
                           bool is_extern, FilePos locus, Scope* scope)
    : Statement(code::FunctionDecl, {arg_list}, locus, scope),
      m_statements(statements),
      m_arg_list(arg_list),
      m_name(name),
      m_return_type(ret),
      m_is_extern(is_extern)
{
    // making sure that arg_list is always the first in the syntax tree!
    for (ASTBase* statement : statements)
    {
        addChildren(statement);
    }
}

const FunctionArgLists::ArgsIter FunctionDecl::getArgBegin() const
{
    return m_arg_list->begin();
}

const FunctionArgLists::ArgsIter FunctionDecl::getArgsEnd() const
{
    return m_arg_list->end();
}

FunctionArgLists::FunctionArgLists(std::vector<TypeInfo>&& args, FilePos locus,
                                   Scope* scope)
    : Statement(code::FunctionArgLists, {}, locus, scope), m_args(args)
{
}

FunctionArgLists::ArgsIter FunctionArgLists::begin() const
{
    return m_args.cbegin();
}

FunctionArgLists::ArgsIter FunctionArgLists::end() const
{
    return m_args.cend();
}

AssignmentStatement::AssignmentStatement(Expression* ref_expr, Expression* expression,
                                         FilePos locus, Scope* scope)
    : Statement(code::AssignmentStatement, {ref_expr, expression}, locus, scope),
      m_ref_expr(ref_expr),
      m_expression(expression)
{
}

ReturnStatement::ReturnStatement(Expression* expression, FilePos locus, Scope* scope)
    : Statement(code::ReturnStatement, {}, locus, scope), m_expression(expression)
{
    // it is possible that expression is null
    if (expression)
        addChildren(expression);
}

IdentifierExpr::IdentifierExpr(const std::string& name, FilePos locus, Scope* scope)
    : LocatorExpression(code::IdentifierExpr, {}, locus, scope), m_name(name)
{
}

ConstantExpr::ConstantExpr(int value, FilePos locus, Scope* scope)
    : Expression(code::ConstantExpr, {}, locus, scope), m_value(value)
{
}

int ConstantExpr::getValue()
{
    return m_value;
}

BinaryExpression::BinaryExpression(Expression* lhs, BinaryExpressionType type,
                                   FilePos locus, Scope* scope)
    : Expression(code::BinaryExpression, {lhs}, locus, scope),
      m_lhs(lhs),
      m_rhs(nullptr),
      m_kind(type)
{
}

BinaryExpression::BinaryExpressionType BinaryExpression::getFromLexType(lex::Token token)
{
    switch (token.getType())
    {
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
            VCC_UNREACHABLE("invalid token type");
            return Add;
    }
}

void BinaryExpression::setRHS(Expression* rhs)
{
    VCC_ASSERT(!m_rhs && "expected it to be a null pointer");
    addChildren(rhs);

    m_rhs = rhs;
}

bool ASTBase::doesDefineScope() const
{
    code::TreeCode code = getCode();
    return code == code::FunctionDecl || code == code::IfStatement ||
        code == code::WhileStatement;
}

const FunctionDecl* ASTBase::getFirstFunctionDecl() const
{
    for (const ASTBase* current = this; current; current = current->getParent())
    {
        const FunctionDecl* decl = nullptr;
        if ((decl = dynamic_cast<const FunctionDecl*>(current)))
            return decl;
    }

    return nullptr;
}

CallExpr::CallExpr(const std::string& name, const std::vector<Expression*>& expression,
                   FilePos locus, Scope* scope)
    : Expression(code::CallExpr, expression, locus, scope),
      m_func_name(name),
      m_expressions(expression)
{
}

IfStatement::IfStatement(Expression* cond, std::vector<Statement*>&& expressions,
                         FilePos locus, Scope* scope)
    : Statement(code::IfStatement, {cond}, locus, scope),
      m_cond(cond),
      m_statements(expressions)
{
    for (ASTBase* expression : expressions)
    {
        addChildren(expression);
    }
}

DeclarationStatement::DeclarationStatement(const std::string& name, Expression* base,
                                           Type* type, FilePos locus, Scope* scope)
    : Statement(code::DeclarationStatement, {}, locus, scope),
      m_expression(base),
      m_name(name),
      m_type(type)
{
    // it is possible that the child is a nullptr, meaning we only have to
    // allocate space
    if (base)
        addChildren(base);
}

const std::string& DeclarationStatement::getName()
{
    return m_name;
}

Type* DeclarationStatement::getType()
{
    return m_type;
}

WhileStatement::WhileStatement(Expression* cond, std::vector<Statement*>&& expression,
                               FilePos locus, Scope* scope)
    : Statement(code::WhileStatement, {cond}, locus, scope),
      m_cond(cond),
      m_statements(expression)
{
    for (ASTBase* base : m_statements)
    {
        addChildren(base);
    }
}

MemberAccessExpression::MemberAccessExpression(const std::string& name,
                                               const std::string& member, FilePos locus,
                                               Scope* scope)
    : LocatorExpression(code::MemberAccessExpression, {}, locus, scope),
      m_base_name(name),
      m_member(member)
{
}

MemberAccessExpression::MemberAccessExpression(LocatorExpression* parent,
                                               const std::string& member, FilePos locus,
                                               Scope* scope)
    : LocatorExpression(code::MemberAccessExpression, {}, locus, scope),
      m_member(member),
      m_parent(parent)
{
    parent->addChildren(this);
}

ArrayAccessExpression::ArrayAccessExpression(const std::string& name,
                                             Expression* expression, FilePos locus,
                                             Scope* scope)
    : LocatorExpression(code::ArrayAccessExpression, {expression}, locus, scope),
      m_index_expression(expression),
      m_base_name(name)
{
}

ArrayAccessExpression::ArrayAccessExpression(LocatorExpression* parent,
                                             Expression* index_expression, FilePos locus,
                                             Scope* scope)
    : LocatorExpression(code::ArrayAccessExpression, {index_expression}, locus, scope),
      m_index_expression(index_expression),
      m_parent_expression(parent)
{
    parent->addChildren(this);
}

LocatorExpression::LocatorExpression(code::TreeCode code,
                                     const std::vector<Expression*>& childrens,
                                     FilePos locus, Scope* scope)
    : Expression(code, childrens, locus, scope)
{
}

Type* ArrayAccessExpression::getGEPChildType(ContextHolder holder)
{
    Type* current_type = getGEPType(holder);
    VCC_ASSERT((current_type->isArray() || current_type->isPointer()) &&
               "array access expression must have valid type!");

    if (PointerType* type = dyncast<PointerType>(current_type))
    {
        return type->getPointee();
    }

    return current_type->getAs<ArrayType>()->getBase();
}

Type* MemberAccessExpression::getGEPChildType(ContextHolder holder)
{
    return getGEPType(holder)->getAs<StructType>()->getElement(m_member).value().type;
}

void DeRefExpression::setPosfixChildExpression(LocatorExpression* expression)
{
    m_posfix_child = expression;
}

void ArrayAccessExpression::setChildPosfixExpression(LocatorExpression* child)
{
    m_child_posfix_expression = child;
}

void MemberAccessExpression::setChildPosfixExpression(LocatorExpression* child)
{
    m_child_posfix_expression = child;
}

Type* FunctionDecl::getReturnType() const
{
    return m_return_type;
}

Expression::Expression(code::TreeCode code, const std::vector<Expression*> children,
                       FilePos locus, Scope* scope)
    : ASTBase(code, children, locus, scope)
{
}

DeRefExpression::DeRefExpression(Expression* ref_get, FilePos locus, Scope* scope)
    : LocatorExpression(code::DeRefExpression, {ref_get}, locus, scope), m_ref(ref_get)
{
}

RefExpression::RefExpression(Expression* inner, FilePos locus, Scope* scope)
    : LocatorExpression(code::RefExpression, {inner}, locus, scope),
      m_inner_expression(inner)
{
}

llvm::FunctionType* FunctionDecl::getFunctionType(ContextHolder holder) const
{
    std::vector<llvm::Type*> args;
    for (auto it = m_arg_list->begin(), ie = m_arg_list->end(); it != ie; ++it)
    {
        args.push_back(it->type->getType(holder));
    }

    llvm::FunctionType* function_type =
        llvm::FunctionType::get(m_return_type->getType(holder), args, /*isVarArg=*/false);
    return function_type;
}

CallStatement::CallStatement(Expression* call_expression, FilePos locus, Scope* scope)
    : Statement(code::CallStatement, {call_expression}, locus, scope),
      m_call_expr(call_expression)
{
}

StringLiteral::StringLiteral(std::string string, FilePos locus, Scope* scope)
    : Expression(code::StringLiteral, {}, locus, scope), m_string_literal(string)
{
}

CastExpression::CastExpression(Expression* cast_expression, Type* casted_to, FilePos loc,
                               Scope* scope)
    : Expression(code::CastExpression, {cast_expression}, loc, scope),
      m_cast_to(casted_to),
      m_to_be_casted_expression(cast_expression)
{
}

static std::vector<DeclarationStatement*> getDeclarationStatementImpl(
    const std::vector<Statement*>& statement)
{
    std::vector<DeclarationStatement*> result;
    for (Statement* s : statement)
    {
        if (s->doesDefineScope())
        {
            switch (s->getCode())
            {
                case vcc::code::WhileStatement:
                {
                    auto other = s->getAs<WhileStatement>()->getDeclarationStatements();
                    result.insert(result.begin(), other.begin(), other.end());
                    break;
                }
                case vcc::code::IfStatement:
                {
                    auto other = s->getAs<IfStatement>()->getDeclarationStatements();
                    result.insert(result.begin(), other.begin(), other.end());
                    break;
                }
                default:
                    std::cerr << "you have missed a case" << std::endl;
                    std::exit(-1);
            }
        }

        if (s->getCode() == code::DeclarationStatement)
            result.push_back(s->getAs<DeclarationStatement>());
    }

    return result;
}

std::vector<DeclarationStatement*> WhileStatement::getDeclarationStatements() const
{
    return getDeclarationStatementImpl(m_statements);
}

std::vector<DeclarationStatement*> IfStatement::getDeclarationStatements() const
{
    return getDeclarationStatementImpl(m_statements);
}

Expression* DeclarationStatement::getExpression()
{
    return m_expression;
}

// ================================================================================
// ====================== Expression Implementation::getType
// ======================
Type* CastExpression::getType(ContextHolder holder)
{
    return m_cast_to;
}

Type* StringLiteral::getType(ContextHolder holder)
{
    return new PointerType(new BuiltinType(BuiltinType::Char));
}

Type* RefExpression::getType(ContextHolder holder)
{
    // FIXME: we can probably prevent a heap allocation every time
    return new PointerType(m_inner_expression->getType(holder));
}

Type* MemberAccessExpression::getGEPType(ContextHolder holder)
{
    if (!m_parent)
        return holder->getLocalVariableTable().lookup(this, m_base_name).value();

    // FIXME: this shares a lot same code with ArrayAccessExpression::getType,
    // maybe we should have a standard interface that solves this entirely?
    if (ArrayAccessExpression* parent = dyncast<ArrayAccessExpression>(m_parent))
    {
        return parent->getGEPChildType(holder);
    }

    if (DeRefExpression* expression = dyncast<DeRefExpression>(m_parent))
    {
        return expression->getInnerType(holder);
    }

    VCC_ASSERT(isa<MemberAccessExpression>(m_parent));
    MemberAccessExpression* parent = dyncast<MemberAccessExpression>(m_parent);
    return parent->getGEPChildType(holder);
}

Type* MemberAccessExpression::getType(ContextHolder holder)
{
    if (m_child_posfix_expression)
        return m_child_posfix_expression->getType(holder);

    // we don't have a child
    return getGEPType(holder)->getAs<StructType>()->getElement(m_member)->type;
}

Type* DeRefExpression::getType(ContextHolder holder)
{
    if (m_posfix_child)
        return m_posfix_child->getType(holder);

    return dyncast<Expression>(m_ref)
        ->getType(holder)
        ->getAs<PointerType>()
        ->getPointee();
}

Type* DeRefExpression::getInnerType(ContextHolder holder)
{
    return dyncast<Expression>(m_ref)
        ->getType(holder)
        ->getAs<PointerType>()
        ->getPointee();
}

Type* ArrayAccessExpression::getType(ContextHolder holder)
{
    if (m_child_posfix_expression)
        return m_child_posfix_expression->getType(holder);

    if (getGEPType(holder)->isBuiltin())
    {
        return getGEPType(holder);
    }
    return getGEPChildType(holder);
}

Type* ConstantExpr::getType(ContextHolder holder)
{
    return new BuiltinType(BuiltinType::Int);
}

Type* IdentifierExpr::getType(ContextHolder holder)
{
    Optional<Type*> result = holder->getLocalVariableTable().lookup(this, m_name);
    if (result.isEmtpy())
    {
        return nullptr;
    }

    return result.value();
}

Type* CallExpr::getType(ContextHolder holder)
{
    std::string callee = getFuncName();
    return holder->getFunctionDeclTable().lookup(this, callee).value()->getReturnType();
}

static Type* getIntWithMoreBits(BuiltinType* lhs, BuiltinType* rhs)
{
    VCC_ASSERT(lhs->isIntegerKind() && rhs->isIntegerKind());
    return lhs->getBitSize() > rhs->getBitSize() ? lhs : rhs;
}

Type* BinaryExpression::getType(ContextHolder holder)
{
    // check for boolean expression
    switch (m_kind)
    {
        // if it is from a boolean expression, it should always return a
        // boolean expression regardless of the two types
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

    if (m_lhs->getType(holder)->isPointer() || m_rhs->getType(holder)->isPointer())
    {
        VCC_UNREACHABLE(
            "please emit error here. pointer type in binary expression "
            "is illform for now!");
        return nullptr;
    }

    if (m_lhs->getType(holder)->isBuiltin() && m_rhs->getType(holder)->isBuiltin())
    {
        BuiltinType* casted_lhs = m_lhs->getType(holder)->getAs<BuiltinType>();
        BuiltinType* casted_rhs = m_rhs->getType(holder)->getAs<BuiltinType>();

        // return float type if either the left hand side or the right hand
        // side have a floating point
        if (casted_lhs->isFloat())
            return casted_lhs;
        else if (casted_rhs->isFloat())
            return casted_rhs;

        return getIntWithMoreBits(casted_lhs, casted_rhs);
    }

    VCC_UNREACHABLE("don't know what to do here!");
    return nullptr;
}

// FIXME: this really should be callled getGEP type and not getType
// getType returns the result of the ast expression, but currently, this
// returns the GEP type
Type* ArrayAccessExpression::getGEPType(ContextHolder holder)
{
    if (!m_parent_expression)
    {
        Type* type = holder->getLocalVariableTable().lookup(this, m_base_name).value();
        if (type->isArray())
            return type->getAs<ArrayType>();

        return type->getAs<PointerType>()->getPointee();
    }

    // trying to get type from parent expression!
    if (ArrayAccessExpression* parent =
            dyncast<ArrayAccessExpression>(m_parent_expression))
    {
        return parent->getGEPChildType(holder);
    }

    VCC_ASSERT(isa<MemberAccessExpression>(m_parent_expression) &&
               "must be member expresion beacuse we have no options left!");
    MemberAccessExpression* parent = dyncast<MemberAccessExpression>(m_parent_expression);
    return parent->getGEPChildType(holder);
}

// ================================================================================
// ====================== Public accessors ========================================

Expression* CallStatement::getCallExpression() const
{
    return m_call_expr;
}

const std::vector<TypeInfo>& FunctionArgLists::getArgs() const
{
    return m_args;
}

bool FunctionDecl::isExtern() const
{
    return m_is_extern;
}

const std::vector<Statement*>& FunctionDecl::getStatements() const
{
    return m_statements;
}

FunctionArgLists* FunctionDecl::getArgList() const
{
    return m_arg_list;
}

Expression* AssignmentStatement::getRefExpression() const
{
    return m_ref_expr;
}

Expression* AssignmentStatement::getExpression() const
{
    return m_expression;
}

Expression* ReturnStatement::getExpression() const
{
    return m_expression;
}

Expression* IfStatement::getCondition() const
{
    return m_cond;
}

const std::vector<Statement*>& IfStatement::getStatements() const
{
    return m_statements;
}

Expression* WhileStatement::getCondition() const
{
    return m_cond;
}

const std::vector<Statement*>& WhileStatement::getStatements() const
{
    return m_statements;
}

const std::string& CallExpr::getFuncName() const
{
    return m_func_name;
}

const std::vector<Expression*>& CallExpr::getExpressions() const
{
    return m_expressions;
}

Expression* BinaryExpression::getLHS() const
{
    return m_lhs;
}

Expression* BinaryExpression::getRHS() const
{
    return m_rhs;
}

BinaryExpression::BinaryExpressionType BinaryExpression::getKind() const
{
    return m_kind;
}

Expression* CastExpression::getCastedExpression() const
{
    return m_to_be_casted_expression;
}

Type* CastExpression::getCastTo() const
{
    return m_cast_to;
}

const std::string& IdentifierExpr::getName() const
{
    return m_name;
}

LocatorExpression* MemberAccessExpression::getParentExpression() const
{
    return m_parent;
}

const std::string& MemberAccessExpression::getBaseName() const
{
    return m_base_name;
}

const std::string& MemberAccessExpression::getMember() const
{
    return m_member;
}

LocatorExpression* MemberAccessExpression::getChildPosfixExpression() const
{
    return m_child_posfix_expression;
}

Expression* ArrayAccessExpression::getIndexExpression() const
{
    return m_index_expression;
}

const std::string& ArrayAccessExpression::getBaseName() const
{
    return m_base_name;
}

LocatorExpression* ArrayAccessExpression::getParentExpression() const
{
    return m_parent_expression;
}

LocatorExpression* ArrayAccessExpression::getChildPosfixExpression() const
{
    return m_child_posfix_expression;
}

Expression* DeRefExpression::getRef() const
{
    return m_ref;
}

LocatorExpression* DeRefExpression::getPosfixChild() const
{
    return m_posfix_child;
}

Expression* RefExpression::getInnerExpression() const
{
    return m_inner_expression;
}

const std::string& StringLiteral::getString() const
{
    return m_string_literal;
}
