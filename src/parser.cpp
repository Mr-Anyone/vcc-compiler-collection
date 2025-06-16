#include "parser.h"
#include "ast.h"
#include <cassert>
#include <iostream>

using lex::Token;

Parser::Parser(const char *filename, ContextHolder context)
    : m_tokenizer(filename), m_context(context) {}

ASTBase *Parser::buildSyntaxTree() { return buildFunctionDecl(); }

static ASTBase *logError(const char *message) {
  std::cerr << "ERROR: " << message << "\n";
  return nullptr;
}

// statements :== <assignment_statement> | <return_statement>
ASTBase *Parser::buildStatement() {
  if (m_tokenizer.getCurrentType() == lex::Ret)
    return buildReturnStatement();

  return buildAssignmentStatement();
}

// function_decl :== 'function', <identifier>, 'gives', <type_qualification>,
//                     <function_args_list>, '{', <expression>+, ''}'
ASTBase *Parser::buildFunctionDecl() {
  if (m_tokenizer.current().getType() != lex::FunctionDecl)
    return logError("function declaration must begin with keyword function");

  // eat function decl
  Token name_token = m_tokenizer.next();
  if (name_token.getType() != lex::Identifier)
    return logError("function declaration does not have identifier");

  std::string name = name_token.getStringLiteral();

  if (m_tokenizer.getNextType() != lex::Gives)
    return logError("function declaration must provide return type");

  // FIXME: it is possible to have other types
  // currently only int is supported
  assert(m_tokenizer.getNextType() == lex::Int);
  FunctionArgLists *arg_list = buildFunctionArgList();

  if (m_tokenizer.getCurrentType() != lex::LeftBrace)
    return logError("expected {");
  m_tokenizer.consume();

  // currently only assignment expression is supported
  ASTBase *exp;
  std::vector<ASTBase *> expressions;
  while ((exp = buildStatement())) {
    assert(exp && "expression must be non nullptr");
    expressions.push_back(exp);
  }

  return new FunctionDecl(std::move(expressions),
                          dynamic_cast<FunctionArgLists *>(arg_list),
                          std::move(name));
}

// assignment_expression :== <identifier>, '=', <integer_literal>, ';'
ASTBase *Parser::buildAssignmentStatement() {
  if (m_tokenizer.getCurrentType() != lex::Identifier)
    return nullptr;

  assert(m_tokenizer.getCurrentType() == lex::Identifier);
  std::string name = m_tokenizer.current().getStringLiteral();
  if (m_tokenizer.getNextType() != lex::Equal)
    return logError("expected =");

  Token number_constant = m_tokenizer.next();
  if (number_constant.getType() != lex::IntegerLiteral)
    return logError("expected integer");

  long long value = number_constant.getIntegerLiteral();
  if (m_tokenizer.getNextType() != lex::SemiColon)
    return logError("expected semi colon");

  m_tokenizer.consume();

  return new AssignmentStatement(name, value);
}

// FIXME: maybe put arg_declaration into its own function?
// function_args_list :== '[', args_declaration+, ']'
//  args_declaration :== <type_qualification> + identifier + ','
FunctionArgLists *Parser::buildFunctionArgList() {
  if (m_tokenizer.getNextType() != lex::LeftBracket) {
    logError("expected [");
    return nullptr;
  }

  // parsing args declaration
  // FIXME: add a way to map token into type qualification
  std::vector<TypeInfo> args{};
  while (m_tokenizer.getNextType() == lex::Int) {
    // lex type_qualification = m_tokenizer.getCurrentType();
    lex::Token next_token = m_tokenizer.next();

    if (next_token.getType() != lex::Identifier) {
      logError("expected [");
      return nullptr;
    }
    std::string name = next_token.getStringLiteral();

    if (m_tokenizer.getNextType() != lex::Comma) {
      logError("expected comma");
      return nullptr;
    }
    args.push_back(TypeInfo{Int32, name});
  }

  if (m_tokenizer.getCurrentType() != lex::RightBracket) {
    logError("expected ]");
    return nullptr;
  }

  // pop this token ]
  m_tokenizer.consume();

  return new FunctionArgLists(std::move(args));
}

ASTBase *Parser::buildReturnStatement() {
  if (m_tokenizer.getCurrentType() != lex::Ret) {
    return logError("expected error");
  }
  m_tokenizer.consume();

  ASTBase *expression = buildExpression();

  return new ReturnStatement(expression);
}

// expression :==  <binary_expression>
ASTBase *Parser::buildExpression() {
  return buildBinaryExpression(/*min_precedence=*/1);
}

// binary_expression :== <trivial_expression> |
//                             <trivial_expression>, <bin_op>,
//                             <binary_expression>
ASTBase *Parser::buildBinaryExpression(int min_precendence) {
  // precedence climbing based parsing:
  // https://eli.thegreenplace.net/2010/01/02/top-down-operator-precedence-parsing
  // result = compute_atom()
  //
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

  ASTBase *result = buildTrivialExpression();
  assert(result && "not sure how can this be false?");

  Token current_operator_token = m_tokenizer.current();
  if (!current_operator_token.isBinaryOperator()) {
    return result;
  }

  int current_precedence_level =
      precedence_level.find(current_operator_token.getType())->second;
  while (current_precedence_level >= min_precendence) {
  current_operator_token = m_tokenizer.current();
    // at the beginning of every loop iteration, the current token
    // must be a binary operator.
  if(!current_operator_token.isBinaryOperator())
      break;

    result = new BinaryExpression(result, BinaryExpression::getFromLexType(current_operator_token));
    m_tokenizer.consume();
    assert(current_operator_token.isBinaryOperator() &&
           "current must be a binary token");

    int next_precedence_level = current_precedence_level + 1;

    ASTBase *rhs = buildBinaryExpression(next_precedence_level);
    dynamic_cast<BinaryExpression*>(result)->setRHS(rhs);
  }

  return result;
}

// trivial_expression :== <identifier> | <call_expression> |
//                             '(', <expression>, ')' | <integer_literal>
ASTBase *Parser::buildTrivialExpression() {
  // FIXME: add call expression
  // <integer_literal>
  if (m_tokenizer.getCurrentType() == lex::IntegerLiteral) {
    ASTBase *value =
        new ConstantExpr(m_tokenizer.current().getIntegerLiteral());
    m_tokenizer.consume();
    return value;
  }

  // <identifier>
  if (m_tokenizer.getCurrentType() == lex::Identifier) {
    ASTBase *value =
        new IdentifierExpr(m_tokenizer.current().getStringLiteral());
    m_tokenizer.consume();

    return value;
  }

  // '(', <expression> , ')'
  if (m_tokenizer.getCurrentType() == lex::LeftParentheses) {
    m_tokenizer.consume(); // consume ; )
    ASTBase *value = buildExpression();

    if (m_tokenizer.getCurrentType() != lex::RightParentheses) {
      return logError("expected )");
    }

    m_tokenizer.consume();
    return value;
  }

  return logError("cannot build trivial expression!");
}
