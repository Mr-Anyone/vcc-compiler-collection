#ifndef PARSER_H
#define PARSER_H

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
  ASTBase *bulidCallExpr();

  // for binary expression
  const static inline std::unordered_map<lex::TokenType, int> precedence_level =
      {
          {lex::Add, 1},
          {lex::Multiply, 1},
  };

  ContextHolder m_context;
  lex::Tokenizer m_tokenizer;
};

#endif
