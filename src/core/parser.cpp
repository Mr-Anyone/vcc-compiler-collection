#include "core/parser.h"

#include "core/ast.h"

using namespace vcc;
using vcc::lex::Token;

/// if the next token is either '.' or '[' we have another posfix expression
inline static bool isFullstopOrLeftBracket(const Token& next)
{
    switch (next.getType())
    {
        case lex::LeftBracket:
        case lex::Fullstop:
            return true;
        default:
            return false;
    }
}

// type_qualification :== 'int' | 'struct', <identifier> |
//                     'array', '(', <integer_literal>, ')',
//                     <type_qualification> | 'ptr', <type_qualification> |
//                     'float' | 'void' | 'char' | 'bool' | 'long' | 'short'
Type* Parser::buildTypeQualification()
{
    switch (tok())
    {
        case lex::Bool:
        {
            consume();
            return new BuiltinType(BuiltinType::Bool);
        }
        case lex::Long:
        {
            consume();
            return new BuiltinType(BuiltinType::Long);
        }
        case lex::Short:
        {
            consume();
            return new BuiltinType(BuiltinType::Short);
        }
        case lex::Char:
        {
            consume();
            return new BuiltinType(BuiltinType::Char);
        }
        case lex::Int:
        {
            consume();
            return new BuiltinType(BuiltinType::Int);
        }
        case lex::Float:
        {
            consume();
            return new BuiltinType(BuiltinType::Float);
        }
        case lex::Void:
        {
            consume();
            return new VoidType();
        }
        case lex::Array:
        {
            if (!at({lex::Array, lex::LeftParentheses, lex::IntegerLiteral}))
                return logError("expected (");

            int count = getIntegerLiteral();
            if (!at({lex::IntegerLiteral, lex::RightParentheses}, /*consume_last=*/true))
                return logError("expected )");

            Type* base = buildTypeQualification();
            return new ArrayType(base, count);
        }
        case lex::Ptr:
        {
            consume();
            Type* pointee = buildTypeQualification();
            return new PointerType(pointee);
        }
        case lex::Struct:
        {
            if (!at({lex::Struct, lex::Identifier}))
                return logError("syntax error");

            std::string struct_name = getTokenString();
            consume();
            VCC_ASSERT(m_struct_defs.find(struct_name) != m_struct_defs.end() &&
                       "undefined reference to struct");
            return m_struct_defs[struct_name];
        }
        default:
            break;
    }

    return nullptr;
}

// struct_definition :== 'struct', <identifier> ,'{'
//                  , {<type_qualification> <identifier>}+, '}'
void Parser::addStructDefinition()
{
    if (!at({lex::Struct, lex::Identifier}))
    {
        logError("invalid syntax");
        return;
    }
    std::string name = getTokenString();

    if (!at({lex::Identifier, lex::LeftBrace}, /*consume_last=*/true))
    {
        logError("invalid syntax");
    }

    // parsing the struct
    std::vector<StructType::Element> elements;
    int element_count = 0;
    while (Type* current = buildTypeQualification())
    {
        if (!is(lex::Identifier))
        {
            logError("expected identifier");
            return;
        }

        // FIXME: we need to check that there are no duplicated name
        std::string name = getTokenString();
        elements.push_back({element_count, name, current});
        if (!at({lex::Identifier, lex::Comma}, /*consume_last=*/true))
        {
            logError("invalid syntax");
            return;
        }

        ++element_count;
    }

    if (!at(lex::RightBrace))
    {
        logError("expected }");
        return;
    }

    // Finally, inserting the element into the table
    VCC_ASSERT(m_struct_defs.find(name) == m_struct_defs.end() &&
               "we have duplicated definition of the same struct");
    m_struct_defs[name] = new StructType(elements, name);
}

// external_decl :== 'extern', 'function', <identifier>,
//     'gives', <type_qualification>, '[', <functin_args_list>, ']';
Statement* Parser::buildExternalDecl()
{
    FilePos locus = m_tokenizer.getPos();
    if (!at(lex::External))
        return logError("expected extern");

    if (!at({lex::FunctionDecl, lex::Identifier}))
        return logError("expected function");
    std::string name = getTokenString();

    if (!at({lex::Identifier, lex::Gives}, /*consume_last=*/true))
        return logError("expected gives");

    Type* return_type          = buildTypeQualification();
    ASTBase* function_arg_list = buildFunctionArgList();

    std::vector<Statement*> statements{};
    return new FunctionDecl(statements, dyncast<FunctionArgLists>(function_arg_list),
                            std::move(name), return_type, /*is_extern*/ true, locus);
}

// top_level :== <function_decl> | <struct_definition> | <external_decl>
const std::vector<Statement*>& Parser::buildSyntaxTree()
{
    VCC_ASSERT(m_top_level_statements.size() == 0 && "can only be called once");
    bool has_failed = false;
    while (!is(lex::EndOfFile) && !has_failed)
    {
        switch (tok())
        {
            case lex::FunctionDecl:
            {
                Statement* base = buildFunctionDecl();
                m_top_level_statements.push_back(base);
                break;
            }
            case lex::Struct:
            {
                addStructDefinition();
                break;
            }
            case lex::External:
            {
                Statement* exteran_decl = buildExternalDecl();
                m_top_level_statements.push_back(exteran_decl);
                break;
            }
            default:
            {
                has_failed = true;
                break;
            }
        }
    }

    if (!is(lex::EndOfFile) || has_failed)
    {
        logError("cannot parse things starting here");
    }

    return m_top_level_statements;
}

inline Parser::ErrorResult Parser::logError(const std::string& message)
{
    m_context->diagnostics.diag(m_tokenizer, message);
    return ErrorResult();
}

// if_statement :== 'if', <expression>, 'then', <statements>+, 'end'
Statement* Parser::buildIfStatement()
{
    ScopeRAIL rail(*this);

    FilePos locus = m_tokenizer.getPos();
    if (!at(lex::If))
        return logError("expected if");

    Expression* cond = buildExpression();

    if (!at(lex::Then))
        return logError("expected then");

    std::vector<Statement*> expressions;
    while (Statement* expression = buildStatement())
        expressions.push_back(expression);

    if (!at(lex::End))
        return logError("expected end");

    return new IfStatement(cond, std::move(expressions), locus);
}

// while_statement :== 'while', <expression> 'then', <statements>+,'end'
Statement* Parser::buildWhileStatement()
{
    ScopeRAIL rail(*this);
    FilePos locus = m_tokenizer.getPos();
    if (!at(lex::While))
        return logError("expected while");

    Expression* cond = buildExpression();

    if (!at(lex::Then))
        return logError("expected then");

    std::vector<Statement*> expressions{};
    while (Statement* statement = buildStatement())
        expressions.push_back(statement);

    if (!at(lex::End))
        return logError("expected end");

    return new WhileStatement(cond, std::move(expressions), locus);
}

// call_statement :== <call_expression>, ';'
Statement* Parser::buildCallStatement()
{
    FilePos locus              = m_tokenizer.getPos();
    Expression* call_expresion = buildCallExpr();
    if (!at(lex::SemiColon))
        return logError("expected semi colon");

    return new CallStatement(call_expresion, locus);
}

// statements :== <assignment_statement> | <return_statement> | <if_statement>
// | <while_statement>
//         | <declaration_statement> | <call_statement>
Statement* Parser::buildStatement()
{
    switch (tok())
    {
        case lex::RightBrace:
            return nullptr;
        case lex::If:
            return buildIfStatement();
        case lex::Ret:
            return buildReturnStatement();
        case lex::While:
            return buildWhileStatement();
        default:
        {
            if (is(lex::Identifier) &&
                m_tokenizer.peek().getType() == lex::LeftParentheses)
                return buildCallStatement();

            if (m_tokenizer.current().isTypeQualification())
                return buildDeclarationStatement();

            return buildAssignmentStatement();
        }
    }
}

// function_decl :== 'function', <identifier>, 'gives', <type_qualification>,
//                     <function_args_list>, '{', <expression>+, ''}'
Statement* Parser::buildFunctionDecl()
{
    ScopeRAIL rail(*this);
    FilePos locus = m_tokenizer.getPos();
    if (!at({lex::FunctionDecl, lex::Identifier}))
        return logError("function declaration must begin with keyword function");

    std::string name = getTokenString();

    if (!at({lex::Identifier, lex::Gives}, /*consume_last=*/true))
        return logError("function declaration must provide return type");

    Type* return_type = buildTypeQualification();

    FunctionArgLists* arg_list = buildFunctionArgList();

    if (!at(lex::LeftBrace))
        return logError("expected {");

    Statement* exp;
    std::vector<Statement*> expressions;
    while ((exp = buildStatement()))
    {
        VCC_ASSERT(exp && "expression must be non nullptr");
        expressions.push_back(exp);
    }

    if (!at(lex::RightBrace))
        return logError("expected }");

    return new FunctionDecl(expressions, arg_list,
                            std::move(name), return_type, /*is_extern*/ false, locus);
}

// assignment_statement :== <trivial_expression> ,'=' <expression>, ';'
Statement* Parser::buildAssignmentStatement()
{
    FilePos locus          = m_tokenizer.getPos();
    LocatorExpression* lhs = dyncast<LocatorExpression>(buildTrivialExpression());
    if (!lhs)
        return nullptr;

    if (!at(lex::Equal))
        return logError("expected =");

    Expression* expression = buildExpression();

    if (!at(lex::SemiColon))
        return logError("expected semi colon");

    return new AssignmentStatement(lhs, expression, locus);
}

// FIXME: maybe put arg_declaration into its own function?
// function_args_list :== '[', args_declaration+, ']'
//  args_declaration :== <type_qualification> + identifier + ','
FunctionArgLists* Parser::buildFunctionArgList()
{
    if (!at(lex::LeftBracket))
    {
        logError("expected [");
        return nullptr;
    }
    FilePos locus = m_tokenizer.getPos();

    // parsing args declaration
    // FIXME: add a way to map token into type qualification
    std::vector<TypeInfo> args{};
    while (m_tokenizer.current().isTypeQualification())
    {
        Type* type = buildTypeQualification();

        if (!is(lex::Identifier))
        {
            logError("expected identifier");
            return nullptr;
        }
        std::string name = getTokenString();

        if (!at({lex::Identifier, lex::Comma}, /*consume_last=*/true))
        {
            logError("expected comma");
            return nullptr;
        }
        args.push_back(TypeInfo{type, name});
    }

    if (!at(lex::RightBracket))
    {
        logError("expected ]");
        return nullptr;
    }

    return new FunctionArgLists(std::move(args), locus);
}

// return_statement :== 'ret', {<expression>} ';'
Statement* Parser::buildReturnStatement()
{
    FilePos locus = m_tokenizer.getPos();
    if (!at(lex::Ret))
        return logError("expected error");

    // Parse an expression if and only if we don't have a ';'
    //  if we have a ';', it is likely that we have a void function type
    Expression* expression = nullptr;
    if (!is(lex::SemiColon))
        expression = buildExpression();

    if (!at(lex::SemiColon))
        return logError("expected ;");

    return new ReturnStatement(expression, locus);
}

// expression :==  <binary_expression>
Expression* Parser::buildExpression()
{
    return buildBinaryExpression(/*min_precedence=*/1);
}

// binary_expression :== <trivial_expression> |
//                             <trivial_expression>, <bin_op>,
//                             <binary_expression>
Expression* Parser::buildBinaryExpression(int min_precendence)
{
    // precedence climbing based parsing:
    // https://eli.thegreenplace.net/2010/01/02/top-down-operator-precedence-parsing
    // result = compute_atom()

    // while cur token is a binary operator with precedence >= min_prec:
    //     prec, assoc = precedence and associativity of current token
    //     if assoc is left:
    //         next_min_prec = prec + 1
    //     else:
    //         next_min_prec = prec
    //     rhs = compute_expr(next_min_prec)
    //     result = compute operator(result, rhs)
    //
    // return result

    Expression* result = buildTrivialExpression();
    VCC_ASSERT(result && "not sure how can this be false?");

    // this is just a trivial expression case
    Token current_operator_token = m_tokenizer.current();
    if (!current_operator_token.isBinaryOperator())
    {
        return result;
    }

    int current_precedence_level =
        precedence_level.find(current_operator_token.getType())->second;
    while (current_precedence_level >= min_precendence)
    {
        current_operator_token = m_tokenizer.current();
        // at the beginning of every loop iteration, the current token
        // must be a binary operator. If this is not a binary operator, it
        // is likely that we are done
        if (!current_operator_token.isBinaryOperator())
            break;

        current_precedence_level =
            precedence_level.find(current_operator_token.getType())->second;

        result = new BinaryExpression(
            result, BinaryExpression::getFromLexType(current_operator_token),
            current_operator_token.getPos());
        consume();

        int next_precedence_level = current_precedence_level + 1;

        Expression* rhs = buildBinaryExpression(next_precedence_level);
        dyncast<BinaryExpression>(result)->setRHS(rhs);
    }

    return result;
}

static void appendChild(LocatorExpression* expression, LocatorExpression* child)
{
    if (ArrayAccessExpression* cool = dyncast<ArrayAccessExpression>(expression))
    {
        cool->setChildPosfixExpression(child);
        return;
    }

    if (DeRefExpression* parent = dyncast<DeRefExpression>(expression))
    {
        parent->setPosfixChildExpression(child);
        return;
    }

    VCC_ASSERT(isa<MemberAccessExpression>(expression));
    MemberAccessExpression* member_access = dyncast<MemberAccessExpression>(expression);
    member_access->setChildPosfixExpression(child);
}

//     <postfix_expression>, '.', <identifier> |
//     <postfix_expression>, '[', <expression>, ']'
LocatorExpression* Parser::buildTailPosfixExpression(LocatorExpression* lhs)
{
    FilePos locus = m_tokenizer.getPos();
    VCC_ASSERT(lhs && "we must have a parent if we made it here");
    VCC_ASSERT(isFullstopOrLeftBracket(m_tokenizer.current()));
    if (is(lex::Fullstop))
    {
        consume();
        if (!is(lex::Identifier))
        {
            logError("expected identifier");
            return nullptr;
        }

        std::string member = getTokenString();
        consume();

        MemberAccessExpression* expression =
            new MemberAccessExpression(lhs, member, locus);
        appendChild(lhs, expression);

        if (isFullstopOrLeftBracket(m_tokenizer.current()))
            buildPosfixExpression(expression);

        return expression;
    }

    // parsing the following:
    //     <postfix_expression>, '[', <expression>, ']'
    VCC_ASSERT(is(lex::LeftBracket) && "we are parsing []");
    if (!at(lex::LeftBracket))
    {
        logError("expected identifier");
        return nullptr;
    }
    Expression* expression = buildExpression();

    if (!at(lex::RightBracket))
    {
        logError("expected identifier");
        return nullptr;
    }
    ArrayAccessExpression* new_expression =
        new ArrayAccessExpression(lhs, expression, locus);
    appendChild(lhs, new_expression);
    if (isFullstopOrLeftBracket(m_tokenizer.current()))
        buildPosfixExpression(new_expression);
    return new_expression;
}

// postfix_expression :== <identifier> | <deref_expression>
//     <postfix_expression>, '.', <identifier> |
//     <postfix_expression>, '[', <expression>, ']'
LocatorExpression* Parser::buildPosfixExpression(LocatorExpression* lhs)
{
    //     <postfix_expression>, '.', <identifier> |
    //     <postfix_expression>, '[', <expression>, ']'
    if (isFullstopOrLeftBracket(m_tokenizer.current()))
    {
        return buildTailPosfixExpression(lhs);
    }
    FilePos filepos = m_tokenizer.getPos();

    // the <deref_expression> case
    if (is(lex::Deref))
    {
        LocatorExpression* deref_expression =
            dyncast<LocatorExpression>(buildDerefExpression());
        return buildTailPosfixExpression(deref_expression);
    }

    // building the base case i.e.
    if (!is(lex::Identifier))
    {
        logError("expected identifier");
        return nullptr;
    }
    std::string name = getTokenString();
    consume();

    if (!is(lex::Fullstop) && !is(lex::LeftBracket))
    {
        logError("expected identifier");
        return nullptr;
    }

    // we must have either [, or ., parse depending on situation
    if (is(lex::LeftBracket))
    {
        consume();
        Expression* expresion = buildExpression();

        if (!at(lex::RightBracket))
        {
            logError("expected either . or [");
            return nullptr;
        }

        ArrayAccessExpression* array_access =
            new ArrayAccessExpression(name, expresion, filepos);
        if (isFullstopOrLeftBracket(m_tokenizer.current()))
            buildPosfixExpression(array_access);
        return array_access;
    }

    VCC_ASSERT(is(lex::Fullstop) &&
               "must be . since we are parsing member lookup expressoin");
    consume();

    if (!is(lex::Identifier))
    {
        logError("expected identifier");
        return nullptr;
    }

    std::string literal = getTokenString();
    consume();

    MemberAccessExpression* access = new MemberAccessExpression(name, literal, filepos);
    if (isFullstopOrLeftBracket(m_tokenizer.current()))
        buildPosfixExpression(access);
    return access;
}

// ref_expression :== 'ref', '<', <trivial_expression>, '>'
Expression* Parser::buildRefExpression()
{
    FilePos locus = m_tokenizer.getPos();
    if (!at({lex::Ref, lex::LessSign}, /*consume_last=*/true))
    {
        logError("expected ref");
        return nullptr;
    }

    Expression* expression = buildTrivialExpression();

    if (!at(lex::GreaterSign))
    {
        logError("expected >");
        return nullptr;
    }

    return new RefExpression(expression, locus);
}

// cast_expression :== 'cast', '<', <type_qualification> '>', '(',
// <expression>,')'
Expression* Parser::buildCastExpression()
{
    FilePos loc = m_tokenizer.getPos();
    if (!at({lex::Cast, lex::LessSign}, /*consume_last=*/true))
        return logError("expected cast expression");

    Type* type = buildTypeQualification();

    if (!at(lex::GreaterSign))
        return logError("expected >");

    if (!at(lex::LeftParentheses))
        return logError("expected (");

    Expression* expression = buildExpression();

    if (!at(lex::RightParentheses))
        return logError("expected )");

    return new CastExpression(expression, type, loc);
}

// trivial_expression :== <identifier> | <call_expression> |
//                             '(', <expression>, ')' | <integer_literal> |
//                             <posfix_expression> | <deref_expression> |
//                             <ref_expression> | <string_literal> |
//                             <cast_expression>
Expression* Parser::buildTrivialExpression()
{
    FilePos locus = m_tokenizer.getPos();
    switch (tok())
    {
      case lex::IntegerLiteral:
      {
          Expression* value = new ConstantExpr(getIntegerLiteral(), locus);
          consume();
          return value;
      }
      case lex::Cast:
          return buildCastExpression();
      case lex::String:
      {
          StringLiteral* string_node = new StringLiteral(getTokenString(), locus);
          consume();
          return string_node;
      }
      // <identifier>
      case lex::Identifier:
      {
        // <identifier>  + '(' means call expr
        if (m_tokenizer.peek().getType() == lex::LeftParentheses)
          return buildCallExpr();

        if (isFullstopOrLeftBracket(m_tokenizer.peek()))
          return buildPosfixExpression();

        Expression* value = new IdentifierExpr(getTokenString(), locus);
        consume();

        return value;
      }
      case lex::LeftParentheses:
      {
          consume();
          Expression* value = buildExpression();

          if (!at(lex::RightParentheses))
          {
              logError("expected )");
              return nullptr;
          }
          return value;
      }
      case lex::Ref:
          return buildRefExpression();
      case lex::Deref:
            return buildDerefExpression();
      default:
            return nullptr;
    }

    return nullptr;
}

// call_expressions :== <identifier>, '(', { <expression> ',' }+,  ')'
Expression* Parser::buildCallExpr()
{
    FilePos locus = m_tokenizer.getPos();
    if (!is(lex::Identifier))
        return logError("expected identfier");

    std::string function_name = getTokenString();

    if (!at({lex::Identifier, lex::LeftParentheses}, /*consume_last=*/true))
        return logError("expected (");

    std::vector<Expression*> expressions;

    // {<expression>, ','} +
    while (!is(lex::RightParentheses))
    {
        Expression* expression = buildExpression();
        expressions.push_back(expression);

        if (!at(lex::Comma))
            return logError("expected ,");
    }

    if (!at(lex::RightParentheses))
        return logError("expected )");

    return new CallExpr(function_name, expressions, locus);
}

ContextHolder Parser::getHolder()
{
    return m_context;
}

bool Parser::haveError() const
{
    return m_context->diagnostics.hasError();
}

// declaration_statement :== <type_qualification>, <identifier>, {'=',
// <expression>} , ';'
Statement* Parser::buildDeclarationStatement()
{
    FilePos locus     = m_tokenizer.getPos();
    Type* parsed_type = buildTypeQualification();

    if (!is(lex::Identifier))
        return logError("expected identifier");

    std::string name = getTokenString();
    consume();

    //  we have this case
    // <type_qualification>, <identifier>, ';'
    if (at(lex::SemiColon))
        return new DeclarationStatement(name, nullptr, parsed_type, locus);

    if (!at(lex::Equal))
        return logError("expected =");

    Expression* expression = buildExpression();

    if (!at(lex::SemiColon))
        return logError("expected ;");

    return new DeclarationStatement(name, expression, parsed_type, locus);
}

// deref_expression :== 'deref', '<', <trivial_expression>, '>'
Expression* Parser::buildDerefExpression()
{
    FilePos locus = m_tokenizer.getPos();
    if (!at({lex::Deref, lex::LessSign}, /*consume_last=*/true))
        return logError("expected deref");

    Expression* ref_get = buildTrivialExpression();

    if (!at(lex::GreaterSign))
        return logError("expected >");

    LocatorExpression* deref_expression = new DeRefExpression(ref_get, locus);
    // FIXME: this is kind of a jank hack to get posfix expression to work
    // with deref expression
    if (isFullstopOrLeftBracket(m_tokenizer.current()))
    {
        buildTailPosfixExpression(deref_expression);
        return deref_expression;
    }

    return deref_expression;
}

Parser::Parser(ContextHolder context) : m_tokenizer(context->stream), m_context(context)
{
    m_root_scope    = Scope::createScope(ScopeType::TranslationUnit);
    m_current_scope = m_root_scope;
}

void Parser::start()
{
    static bool started_before = false;
    if (!started_before)
    {
        buildSyntaxTree();
        started_before = true;
    }
}

lex::TokenType Parser::tok()
{
    return m_tokenizer.getCurrentType();
}

bool Parser::is(lex::TokenType type)
{
    return tok() == type;
}

bool Parser::at(lex::TokenType type)
{
    if (tok() != type)
        return false;

    consume();
    return true;
}

bool Parser::at(std::initializer_list<lex::TokenType> list, bool consume_last)
{
    if (list.size() == 0)
        return true;

    // First we check the current, and peak into the remaining
    auto it = list.begin();
    if (tok() != *it)
        return false;
    ++it;

    for (int i = 1; i < list.size() - 1; ++i)
    {
        if (*it != m_tokenizer.peek(i).getType())
        {
            return false;
        }
    }

    // consume the remaining
    for (int i = 0; i < list.size() - 1; ++i)
    {
        m_tokenizer.consume();
    }

    if (consume_last)
    {
        m_tokenizer.consume();
    }
    return true;
}

void Parser::consume()
{
    m_tokenizer.consume();
}

std::string Parser::getTokenString()
{
    VCC_ASSERT(tok() == lex::Identifier || tok() == lex::String);
    return m_tokenizer.current().getStringLiteral();
}

int Parser::getIntegerLiteral()
{
    VCC_ASSERT(tok() == lex::IntegerLiteral);
    return m_tokenizer.current().getIntegerLiteral();
}

const std::vector<Statement*>& Parser::getSyntaxTree()
{
    return m_top_level_statements;
}

/// For Scope
Parser::ScopeRAIL::ScopeRAIL(Parser& other) : parser(other)
{
    parser.m_current_scope = Scope::createScope(parser.tok(), parser.m_current_scope);
}

Parser::ScopeRAIL::~ScopeRAIL()
{
    parser.m_current_scope = parser.m_current_scope->parent();
    assert(parser.m_current_scope && "scope can never be a null pointer");
}

Scope* Parser::getCurScope()
{
    return m_current_scope;
}
