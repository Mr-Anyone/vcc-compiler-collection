#ifndef PARSER_H
#define PARSER_H

#include "sema.h"
#include "ast.h"
#include "context.h"
#include "lex.h"

#include <istream>

class Parser {
public:
  Parser(const char *filename, ContextHolder context);

  // nullptr if failed
  ASTBase *buildSyntaxTree();

private:
  // building the function decl
  ASTBase *buildFunctionDecl();
  FunctionArgLists *buildFunctionArgList();

  // Statements
  ASTBase *buildAssignmentStatement();
  ASTBase *buildReturnStatement();
  ASTBase *buildStatement();

  // Expressions
  ASTBase *buildExpression();
  ASTBase *buildBinaryExpression(int min_precedence);
  ASTBase *buildTrivialExpression();
  ASTBase *buildCallExpr();

  inline ASTBase *logError(const char *message, lex::Token current_token);

  // for binary expression
  const static inline std::unordered_map<lex::TokenType, int> precedence_level =
      {
          {lex::Add, 1},
          {lex::Multiply, 2},
  };

  ContextHolder m_context;
  lex::Tokenizer m_tokenizer;
  Sema m_actions;
};

#endif
