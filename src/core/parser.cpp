#include <cassert>
#include <iostream>

#include "core/parser.h"
#include "core/ast.h"

using namespace vcc;
using vcc::lex::Token;

Parser::Parser(ContextHolder context)
    : m_tokenizer(context->stream), m_context(context) {}

void Parser::start() {
  static bool started_before = false;
  if (!started_before) {
    buildSyntaxTree();
    started_before = true;
  }
}

/// if the next token is either '.' or '[' we have another posfix expression
inline static bool isFullstopOrLeftBracket(const Token &next) {
  if (next.getType() == lex::Fullstop || next.getType() == lex::LeftBracket)
    return true;

  return false;
}

const std::vector<Statement *> &Parser::getSyntaxTree() {
  return m_top_level_statements;
}

// type_qualification :== 'int' | 'struct', <identifier> |
//                     'array', '(', <integer_literal>, ')',
//                     <type_qualification> | 'ptr', <type_qualification> |
//                     'float' | 'void' | 'char' | 'bool' | 'long' | 'short'
Type *Parser::buildTypeQualification() {
  // we have a boolean type here
  if (m_tokenizer.getCurrentType() == lex::Bool) {
    m_tokenizer.consume();
    return new BuiltinType(BuiltinType::Bool);
  }

  if (m_tokenizer.getCurrentType() == lex::Long) {
    m_tokenizer.consume();
    return new BuiltinType(BuiltinType::Long);
  }

  if (m_tokenizer.getCurrentType() == lex::Short) {
    m_tokenizer.consume();
    return new BuiltinType(BuiltinType::Long);
  }

  // we have void type 'void'
  if (m_tokenizer.getCurrentType() == lex::Void) {
    m_tokenizer.consume();
    return new VoidType();
  }

  // 'char'
  if (m_tokenizer.getCurrentType() == lex::Char) {
    m_tokenizer.consume();
    return new BuiltinType(BuiltinType::Char);
  }

  // we have an array type
  // 'array', '(', <integer_literal>, ')', <type_qualification>
  if (m_tokenizer.getCurrentType() == lex::Array) {
    if (m_tokenizer.getNextType() != lex::LeftParentheses) {
      return logError("expectex (");
    }
    m_tokenizer.consume();

    if (m_tokenizer.getCurrentType() != lex::IntegerLiteral) {
      return logError("expected integer");
    }
    int count = m_tokenizer.current().getIntegerLiteral();
    m_tokenizer.consume();

    if (m_tokenizer.getCurrentType() != lex::RightParentheses) {
      return logError("expected )");
    }
    m_tokenizer.consume();

    Type *base = buildTypeQualification();
    return new ArrayType(base, count);
  }

  // 'ptr', <type_qualification>
  if (m_tokenizer.getCurrentType() == lex::Ptr) {
    m_tokenizer.consume();
    Type *pointee = buildTypeQualification();
    return new PointerType(pointee);
  }

  // we have builtin
  if (m_tokenizer.getCurrentType() == lex::Int) {
    m_tokenizer.consume();

    return new BuiltinType(BuiltinType::Int);
  }

  // we have float builtin
  if (m_tokenizer.getCurrentType() == lex::Float) {
    m_tokenizer.consume();

    return new BuiltinType(BuiltinType::Float);
  }
  // we have a structure
  if (m_tokenizer.getCurrentType() == lex::Struct) {
    m_tokenizer.consume();
    if (m_tokenizer.getCurrentType() != lex::Identifier) {
      return logError("expected identifier");
    }

    std::string struct_name = m_tokenizer.current().getStringLiteral();
    m_tokenizer.consume();

    assert(m_struct_defs.contains(struct_name) &&
           "undefined reference to struct");
    return m_struct_defs[struct_name];
  }

  return nullptr;
}

// struct_definition :== 'struct', <identifier> ,'{'
//                  , {<type_qualification> <identifier>}+, '}'
void Parser::addStructDefinition() {
  if (m_tokenizer.getCurrentType() != lex::Struct) {
    logError("expected struct");
    return;
  }

  if (m_tokenizer.getNextType() != lex::Identifier) {
    logError("expected identifier");
    return;
  }
  std::string name = m_tokenizer.current().getStringLiteral();

  if (m_tokenizer.getNextType() != lex::LeftBrace) {
    logError("expected {");
    return;
  }
  m_tokenizer.consume();

  // parsing the struct
  std::vector<StructType::Element> elements;
  int element_count = 0;
  while (Type *current = buildTypeQualification()) {
    if (m_tokenizer.getCurrentType() != lex::Identifier) {
      logError("expected identifier");
      return;
    }

    // FIXME: we need to check that there are no duplicated name
    std::string name = m_tokenizer.current().getStringLiteral();
    elements.push_back({element_count, name, current});
    if (m_tokenizer.getNextType() != lex::Comma) {
      logError("expected ,");
      return;
    }
    m_tokenizer.consume();

    ++element_count;
  }

  if (m_tokenizer.getCurrentType() != lex::RightBrace) {
    logError("expected }");
    return;
  }

  m_tokenizer.consume();

  // Finally, inserting the element into the table
  assert(!m_struct_defs.contains(name) &&
         "we have duplicated definition of the same struct");
  m_struct_defs[name] = new StructType(elements, name);
}

// external_decl :== 'extern', 'function', <identifier>,
//     'gives', <type_qualification>, '[', <functin_args_list>, ']';
Statement *Parser::buildExternalDecl() {
  if (m_tokenizer.getCurrentType() != lex::External)
    return logError("expected extern");
  FilePos locus = m_tokenizer.getPos();
  m_tokenizer.consume();

  if (m_tokenizer.getCurrentType() != lex::FunctionDecl)
    return logError("expected function");
  m_tokenizer.consume();

  if (m_tokenizer.getCurrentType() != lex::Identifier)
    return logError("expected identifier");
  std::string name = m_tokenizer.current().getStringLiteral();
  m_tokenizer.consume();

  if (m_tokenizer.getCurrentType() != lex::Gives)
    return logError("expected gives");
  m_tokenizer.consume();

  Type *return_type = buildTypeQualification();
  ASTBase *function_arg_list = buildFunctionArgList();

  std::vector<Statement *> statements{};

  return new FunctionDecl(
      statements, dyncast<FunctionArgLists>(function_arg_list), std::move(name),
      return_type, /*is_extern*/ true, locus);
}

// top_level :== <function_decl> | <struct_definition> | <external_decl>
const std::vector<Statement *> &Parser::buildSyntaxTree() {
  assert(m_top_level_statements.size() == 0 && "can only be called once");
  while (m_tokenizer.getCurrentType() != lex::EndOfFile) {
    if (m_tokenizer.getCurrentType() == lex::FunctionDecl) {
      Statement *base = buildFunctionDecl();
      m_top_level_statements.push_back(base);
      continue;
    }

    if (m_tokenizer.getCurrentType() == lex::Struct) {
      addStructDefinition();
      continue;
    }

    if (m_tokenizer.getCurrentType() == lex::External) {
      Statement *exteran_decl = buildExternalDecl();
      m_top_level_statements.push_back(exteran_decl);
      continue;
    }

    break;
  }

  if (m_tokenizer.getCurrentType() != lex::EndOfFile) {
    logError("cannot parse things starting here");
  }

  return m_top_level_statements;
}

inline Parser::ErrorResult Parser::logError(const std::string &message) {
  m_context->diagnostics.diag(m_tokenizer, message);
  return ErrorResult();
}

// if_statement :== 'if', <expression>, 'then', <statements>+, 'end'
Statement *Parser::buildIfStatement() {
  if (m_tokenizer.getCurrentType() != lex::If)
    return logError("expected if");

  FilePos locus = m_tokenizer.getPos();
  m_tokenizer.consume();

  Expression *cond = buildExpression();

  if (m_tokenizer.getCurrentType() != lex::Then)
    return logError("expected then");
  m_tokenizer.consume();

  std::vector<Statement *> expressions;
  while (Statement *expression = buildStatement()) {
    expressions.push_back(expression);
  }

  if (m_tokenizer.getCurrentType() != lex::End)
    return logError("expected end");
  m_tokenizer.consume();

  return new IfStatement(cond, std::move(expressions), locus);
}

// while_statement :== 'while', <expression> 'then', <statements>+,'end'
Statement *Parser::buildWhileStatement() {
  if (m_tokenizer.getCurrentType() != lex::While)
    return logError("expected while");
  m_tokenizer.consume();
  FilePos locus = m_tokenizer.getPos();

  Expression *cond = buildExpression();

  if (m_tokenizer.getCurrentType() != lex::Then)
    return logError("expected then");
  m_tokenizer.consume();

  std::vector<Statement *> expressions{};
  while (Statement *statement = buildStatement())
    expressions.push_back(statement);

  if (m_tokenizer.getCurrentType() != lex::End)
    return logError("expected end");
  m_tokenizer.consume();

  return new WhileStatement(cond, std::move(expressions), locus);
}

// call_statement :== <call_expression>, ';'
Statement *Parser::buildCallStatement() {
  FilePos locus = m_tokenizer.getPos();
  Expression *call_expresion = buildCallExpr();
  if (m_tokenizer.getCurrentType() != lex::SemiColon) {
    return logError("expected semi colon");
  }
  m_tokenizer.consume();

  return new CallStatement(call_expresion, locus);
}

// statements :== <assignment_statement> | <return_statement> | <if_statement> |
// <while_statement>
//         | <declaration_statement> | <call_statement>
Statement *Parser::buildStatement() {
  if (m_tokenizer.getCurrentType() == lex::RightBrace)
    return nullptr;

  if (m_tokenizer.getCurrentType() == lex::If)
    return buildIfStatement();

  if (m_tokenizer.getCurrentType() == lex::Ret)
    return buildReturnStatement();

  if (m_tokenizer.current().isTypeQualification())
    return buildDeclarationStatement();

  if (m_tokenizer.getCurrentType() == lex::Identifier &&
      m_tokenizer.peek().getType() == lex::LeftParentheses) {
    return buildCallStatement();
  }

  if (m_tokenizer.getCurrentType() == lex::While)
    return buildWhileStatement();

  return buildAssignmentStatement();
}

// function_decl :== 'function', <identifier>, 'gives', <type_qualification>,
//                     <function_args_list>, '{', <expression>+, ''}'
Statement *Parser::buildFunctionDecl() {
  if (m_tokenizer.current().getType() != lex::FunctionDecl)
    return logError("function declaration must begin with keyword function");

  FilePos locus = m_tokenizer.getPos();
  // eat function decl
  Token name_token = m_tokenizer.next();
  if (name_token.getType() != lex::Identifier)
    return logError("function declaration does not have identifier");

  std::string name = name_token.getStringLiteral();

  if (m_tokenizer.getNextType() != lex::Gives)
    return logError("function declaration must provide return type");
  m_tokenizer.consume();

  Type *return_type = buildTypeQualification();

  FunctionArgLists *arg_list = buildFunctionArgList();

  if (m_tokenizer.getCurrentType() != lex::LeftBrace)
    return logError("expected {");
  m_tokenizer.consume();

  // currently only assignment expression is supported
  Statement *exp;
  std::vector<Statement *> expressions;
  while ((exp = buildStatement())) {
    assert(exp && "expression must be non nullptr");
    expressions.push_back(exp);
  }

  if (m_tokenizer.getCurrentType() != lex::RightBrace)
    return logError("expected }");
  m_tokenizer.consume();

  return new FunctionDecl(
      expressions, dynamic_cast<FunctionArgLists *>(arg_list), std::move(name),
      return_type, /*is_extern*/ false, locus);
}

// assignment_statement :== <trivial_expression> ,'=' <expression>, ';'
Statement *Parser::buildAssignmentStatement() {
  FilePos locus = m_tokenizer.getPos();
  LocatorExpression *lhs = dyncast<LocatorExpression>(buildTrivialExpression());
  if (!lhs)
    return nullptr;

  if (m_tokenizer.getCurrentType() != lex::Equal)
    return logError("expected =");
  m_tokenizer.consume();

  Expression *expression = buildExpression();

  if (m_tokenizer.getCurrentType() != lex::SemiColon)
    return logError("expected semi colon");

  m_tokenizer.consume();

  return new AssignmentStatement(lhs, expression, locus);
}

// FIXME: maybe put arg_declaration into its own function?
// function_args_list :== '[', args_declaration+, ']'
//  args_declaration :== <type_qualification> + identifier + ','
FunctionArgLists *Parser::buildFunctionArgList() {
  if (m_tokenizer.getCurrentType() != lex::LeftBracket) {
    logError("expected [");
    return nullptr;
  }
  m_tokenizer.consume();
  FilePos locus = m_tokenizer.getPos();

  // parsing args declaration
  // FIXME: add a way to map token into type qualification
  std::vector<TypeInfo> args{};
  while (m_tokenizer.current().isTypeQualification()) {
    Type *type = buildTypeQualification();
    lex::Token next_token = m_tokenizer.current();

    if (next_token.getType() != lex::Identifier) {
      logError("expected identifier");
      return nullptr;
    }
    std::string name = next_token.getStringLiteral();

    if (m_tokenizer.getNextType() != lex::Comma) {
      logError("expected comma");
      return nullptr;
    }
    args.push_back(TypeInfo{type, name});
    m_tokenizer.consume();
  }

  if (m_tokenizer.getCurrentType() != lex::RightBracket) {
    logError("expected ]");
    return nullptr;
  }

  // pop this token ]
  m_tokenizer.consume();

  return new FunctionArgLists(std::move(args), locus);
}

// return_statement :== 'ret', {<expression>} ';'
Statement *Parser::buildReturnStatement() {
  FilePos locus = m_tokenizer.getPos();
  if (m_tokenizer.getCurrentType() != lex::Ret) {
    return logError("expected error");
  }
  m_tokenizer.consume();

  Expression *expression = nullptr;
  // Parser an expression if and only if we don't have an ';'
  //  if we have an ';', it is likely that we have a void function type
  if (m_tokenizer.getCurrentType() != lex::SemiColon)
    expression = buildExpression();

  if (m_tokenizer.getCurrentType() != lex::SemiColon) {
    return logError("expected ;");
  }
  m_tokenizer.consume();

  return new ReturnStatement(expression, locus);
}

// expression :==  <binary_expression>
Expression *Parser::buildExpression() {
  return buildBinaryExpression(/*min_precedence=*/1);
}

// binary_expression :== <trivial_expression> |
//                             <trivial_expression>, <bin_op>,
//                             <binary_expression>
Expression *Parser::buildBinaryExpression(int min_precendence) {
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

  Expression *result = buildTrivialExpression();
  assert(result && "not sure how can this be false?");

  // this is just a trivial expression case
  Token current_operator_token = m_tokenizer.current();
  if (!current_operator_token.isBinaryOperator()) {
    return result;
  }

  int current_precedence_level =
      precedence_level.find(current_operator_token.getType())->second;
  while (current_precedence_level >= min_precendence) {
    current_operator_token = m_tokenizer.current();
    // at the beginning of every loop iteration, the current token
    // must be a binary operator. If this is not a binary operator, it is likely
    // that
    // we are done
    if (!current_operator_token.isBinaryOperator())
      break;

    current_precedence_level =
        precedence_level.find(current_operator_token.getType())->second;

    result = new BinaryExpression(
        result, BinaryExpression::getFromLexType(current_operator_token),
        current_operator_token.getPos());
    m_tokenizer.consume(); // consume the binary token

    int next_precedence_level = current_precedence_level + 1;

    Expression *rhs = buildBinaryExpression(next_precedence_level);
    dynamic_cast<BinaryExpression *>(result)->setRHS(rhs);
  }

  return result;
}

static void appendChild(LocatorExpression *expression,
                        LocatorExpression *child) {
  if (ArrayAccessExpression *cool =
          dyncast<ArrayAccessExpression>(expression)) {
    cool->setChildPosfixExpression(child);
    return;
  }

  if (DeRefExpression *parent = dyncast<DeRefExpression>(expression)) {
    parent->setPosfixChildExpression(child);
    return;
  }

  assert(isa<MemberAccessExpression>(expression));
  MemberAccessExpression *member_access =
      dyncast<MemberAccessExpression>(expression);
  member_access->setChildPosfixExpression(child);
}

//     <postfix_expression>, '.', <identifier> |
//     <postfix_expression>, '[', <expression>, ']'
LocatorExpression *Parser::buildTailPosfixExpression(LocatorExpression *lhs) {
  FilePos locus = m_tokenizer.getPos();
  assert(lhs && "we must have a parent if we made it here");
  assert(isFullstopOrLeftBracket(m_tokenizer.current()));
  if (m_tokenizer.getCurrentType() == lex::Fullstop) {
    m_tokenizer.consume();
    if (m_tokenizer.getCurrentType() != lex::Identifier) {
      logError("expected identifier");
      return nullptr;
    }

    std::string member = m_tokenizer.current().getStringLiteral();
    m_tokenizer.consume();

    MemberAccessExpression *expression =
        new MemberAccessExpression(lhs, member, locus);
    appendChild(lhs, expression); // building the syntax tree

    if (isFullstopOrLeftBracket(m_tokenizer.current()))
      buildPosfixExpression(expression);

    return expression;
  }

  // parsing the following:
  //     <postfix_expression>, '[', <expression>, ']'
  assert(m_tokenizer.getCurrentType() == lex::LeftBracket &&
         "we are parsing []");
  if (m_tokenizer.getCurrentType() != lex::LeftBracket) {
    logError("expected identifier");
    return nullptr;
  }
  m_tokenizer.consume();
  Expression *expression = buildExpression();

  if (m_tokenizer.getCurrentType() != lex::RightBracket) {
    logError("expected identifier");
    return nullptr;
  }
  m_tokenizer.consume();
  ArrayAccessExpression *new_expression =
      new ArrayAccessExpression(lhs, expression, locus);
  appendChild(lhs, new_expression); // building the syntax tree
  if (isFullstopOrLeftBracket(m_tokenizer.current()))
    buildPosfixExpression(new_expression);
  return new_expression;
}

// postfix_expression :== <identifier> | <deref_expression>
//     <postfix_expression>, '.', <identifier> |
//     <postfix_expression>, '[', <expression>, ']'
LocatorExpression *Parser::buildPosfixExpression(LocatorExpression *lhs) {
  //     <postfix_expression>, '.', <identifier> |
  //     <postfix_expression>, '[', <expression>, ']'
  if (isFullstopOrLeftBracket(m_tokenizer.current())) {
    return buildTailPosfixExpression(lhs);
  }
  FilePos filepos = m_tokenizer.getPos();

  // the <deref_expression> case
  if (m_tokenizer.getCurrentType() == lex::Deref) {
    LocatorExpression *deref_expression =
        dyncast<LocatorExpression>(buildDerefExpression());
    return buildTailPosfixExpression(deref_expression);
  }

  // building the base case i.e.
  if (m_tokenizer.getCurrentType() != lex::Identifier) {
    logError("expected identifier");
    return nullptr;
  }
  std::string name = m_tokenizer.current().getStringLiteral();
  m_tokenizer.consume();

  if (m_tokenizer.getCurrentType() != lex::Fullstop &&
      m_tokenizer.getCurrentType() != lex::LeftBracket) {
    logError("expected identifier");
    return nullptr;
  }

  // we must have either [, or ., parse depending on situation
  if (m_tokenizer.getCurrentType() == lex::LeftBracket) {
    m_tokenizer.consume();
    Expression *expresion = buildExpression();

    if (m_tokenizer.getCurrentType() != lex::RightBracket) {
      logError("expected either . or [");
      return nullptr;
    }
    m_tokenizer.consume();

    ArrayAccessExpression *array_access =
        new ArrayAccessExpression(name, expresion, filepos);
    if (isFullstopOrLeftBracket(m_tokenizer.current()))
      buildPosfixExpression(array_access);
    return array_access;
  }

  assert(m_tokenizer.getCurrentType() == lex::Fullstop &&
         "must be . since we are parsing member lookup expressoin");
  m_tokenizer.consume();

  if (m_tokenizer.getCurrentType() != lex::Identifier) {
    logError("expected identifier");
    return nullptr;
  }

  std::string literal = m_tokenizer.current().getStringLiteral();
  m_tokenizer.consume();

  MemberAccessExpression *access =
      new MemberAccessExpression(name, literal, filepos);
  if (isFullstopOrLeftBracket(m_tokenizer.current()))
    buildPosfixExpression(access);
  return access;
}

// ref_expression :== 'ref', '<', <trivial_expression>, '>'
Expression *Parser::buildRefExpression() {
  FilePos locus = m_tokenizer.getPos();
  if (m_tokenizer.getCurrentType() != lex::Ref) {
    logError("expected ref");
    return nullptr;
  }

  if (m_tokenizer.getNextType() != lex::LessSign) {
    logError("expected <");
    return nullptr;
  }
  m_tokenizer.consume();

  Expression *expression = buildTrivialExpression();

  if (m_tokenizer.getCurrentType() != lex::GreaterSign) {
    logError("expected >");
    return nullptr;
  }
  m_tokenizer.consume();

  return new RefExpression(expression, locus);
}

// cast_expression :== 'cast', '<', <type_qualification> '>', '(',
// <expression>,')'
Expression *Parser::buildCastExpression() {
  FilePos loc = m_tokenizer.getPos();
  if (m_tokenizer.getCurrentType() != lex::Cast)
    return logError("expected cast expression");
  m_tokenizer.consume();

  if (m_tokenizer.getCurrentType() != lex::LessSign) {
    return logError("expected <");
  }
  m_tokenizer.consume();

  Type *type = buildTypeQualification();

  if (m_tokenizer.getCurrentType() != lex::GreaterSign) {
    return logError("expected >");
  }
  m_tokenizer.consume();

  if (m_tokenizer.getCurrentType() != lex::LeftParentheses)
    return logError("expected (");
  m_tokenizer.consume();

  Expression *expression = buildExpression();

  if (m_tokenizer.getCurrentType() != lex::RightParentheses)
    return logError("expected )");
  m_tokenizer.consume();

  return new CastExpression(expression, type, loc);
}

// trivial_expression :== <identifier> | <call_expression> |
//                             '(', <expression>, ')' | <integer_literal> |
//                             <posfix_expression> | <deref_expression> |
//                             <ref_expression> | <string_literal> |
//                             <cast_expression>
Expression *Parser::buildTrivialExpression() {
  FilePos locus = m_tokenizer.getPos();
  // <integer_literal>
  if (m_tokenizer.getCurrentType() == lex::IntegerLiteral) {
    Expression *value =
        new ConstantExpr(m_tokenizer.current().getIntegerLiteral(), locus);
    m_tokenizer.consume();
    return value;
  }

  if (m_tokenizer.getCurrentType() == lex::Cast)
    return buildCastExpression();

  // FIXME: maybe we should move this into it's own function?
  if (m_tokenizer.getCurrentType() == lex::String) {
    StringLiteral *string_node =
        new StringLiteral(m_tokenizer.current().getStringLiteral(), locus);
    m_tokenizer.consume();
    return string_node;
  }

  // <identifier>
  if (m_tokenizer.getCurrentType() == lex::Identifier) {
    // <identifier>  + '(' means call expr
    if (m_tokenizer.peek().getType() == lex::LeftParentheses)
      return buildCallExpr();

    if (isFullstopOrLeftBracket(m_tokenizer.peek())) {
      return buildPosfixExpression();
    }

    Expression *value =
        new IdentifierExpr(m_tokenizer.current().getStringLiteral(), locus);
    m_tokenizer.consume();

    return value;
  }

  // '(', <expression> , ')'
  if (m_tokenizer.getCurrentType() == lex::LeftParentheses) {
    m_tokenizer.consume(); // consume ; )
    Expression *value = buildExpression();

    if (m_tokenizer.getCurrentType() != lex::RightParentheses) {
      logError("expected )");
      return nullptr;
    }

    m_tokenizer.consume();
    return value;
  }

  if (m_tokenizer.getCurrentType() == lex::Ref)
    return buildRefExpression();

  if (m_tokenizer.getCurrentType() == lex::Deref)
    return buildDerefExpression();

  return nullptr;
}

// call_expressions :== <identifier>, '(', { <expression> ',' }+,  ')'
Expression *Parser::buildCallExpr() {
  FilePos locus = m_tokenizer.getPos();
  if (m_tokenizer.getCurrentType() != lex::Identifier) {
    return logError("expected identfier");
  }

  std::string function_name = m_tokenizer.current().getStringLiteral();

  if (m_tokenizer.getNextType() != lex::LeftParentheses) {
    return logError("expected (");
  }
  m_tokenizer.consume();

  std::vector<Expression *> expressions;

  // {<expression>, ','} +
  while (m_tokenizer.getCurrentType() != lex::RightParentheses) {
    Expression *expression = buildExpression();
    expressions.push_back(expression);

    // consume the comma
    if (m_tokenizer.getCurrentType() != lex::Comma) {
      return logError("expected ,");
    }

    m_tokenizer.consume();
  }

  if (m_tokenizer.getCurrentType() != lex::RightParentheses) {
    return logError("expected )");
  }
  m_tokenizer.consume();

  return new CallExpr(function_name, expressions, locus);
}

ContextHolder Parser::getHolder() { return m_context; }

bool Parser::haveError() const { return m_context->diagnostics.hasError(); }

// declaration_statement :== <type_qualification>, <identifier>, {'=',
// <expression>} , ';'
Statement *Parser::buildDeclarationStatement() {
  FilePos locus = m_tokenizer.getPos();
  Type *parsed_type = buildTypeQualification();

  if (m_tokenizer.getCurrentType() != lex::Identifier)
    return logError("expected identifier");

  std::string name = m_tokenizer.current().getStringLiteral();

  //  we have this case
  // <type_qualification>, <identifier>, ';'
  if (m_tokenizer.getNextType() == lex::SemiColon) {
    m_tokenizer.consume(); // applying the side effect
    return new DeclarationStatement(name, nullptr, parsed_type, locus);
  }

  if (m_tokenizer.getCurrentType() != lex::Equal)
    return logError("expected =");
  m_tokenizer.consume();

  Expression *expression = buildExpression();

  if (m_tokenizer.getCurrentType() != lex::SemiColon)
    return logError("expected ;");
  m_tokenizer.consume();

  return new DeclarationStatement(name, expression, parsed_type, locus);
}

// deref_expression :== 'deref', '<', <trivial_expression>, '>'
Expression *Parser::buildDerefExpression() {
  FilePos locus = m_tokenizer.getPos();
  if (m_tokenizer.getCurrentType() != lex::Deref) {
    return logError("expected deref");
  }
  m_tokenizer.consume();

  if (m_tokenizer.getCurrentType() != lex::LessSign) {
    return logError("expected <");
  }

  m_tokenizer.consume();

  Expression *ref_get = buildTrivialExpression();

  if (m_tokenizer.getCurrentType() != lex::GreaterSign) {
    return logError("expected >");
  }
  m_tokenizer.consume();

  LocatorExpression *deref_expression = new DeRefExpression(ref_get, locus);
  // FIXME: this is kind of a jank hack to get posfix expression to work
  // with deref expression
  if (isFullstopOrLeftBracket(m_tokenizer.current())) {
    buildTailPosfixExpression(deref_expression);
    return deref_expression;
  }

  return deref_expression;
}
