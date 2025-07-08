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

  void start();
  const std::vector<ASTBase*>& getSyntaxTree();
  ContextHolder getHolder();
  bool haveError() const;

private:
  const std::vector<ASTBase*>&  buildSyntaxTree();

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
          // eq, ne, gt, ge, lt, le
          {lex::EqualKeyword, 1},
          {lex::NEquals, 1},
          {lex::GreaterThan, 1},
          {lex::GreaterEqual, 1},
          {lex::LessThan, 1},
          {lex::LessEqual, 1},

          {lex::Add, 2},
          {lex::Multiply, 3},
  };

  ContextHolder m_context;
  lex::Tokenizer m_tokenizer;
  Sema m_actions;

  bool m_error = false;

  std::vector<ASTBase*> m_function_decls;
};

#endif
