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

  ASTBase *buildAssignmentStatement();
  ASTBase *buildReturnStatement();
  ASTBase *buildStatement();

  ASTBase *nextTokenOrError(lex::TokenType expected_token, const char *message);

  ContextHolder m_context;
  lex::Tokenizer m_tokenizer;
};

#endif
