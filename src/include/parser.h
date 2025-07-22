#ifndef PARSER_H
#define PARSER_H

#include "ast.h"
#include "context.h"
#include "lex.h"
#include "sema.h"

#include "type.h"

class Parser {
public:
  Parser(const char *filename, ContextHolder context);

  void start();
  const std::vector<ASTBase *> &getSyntaxTree();
  ContextHolder getHolder();
  bool haveError() const;

private:
  const std::vector<ASTBase *> &buildSyntaxTree();

  // Types, kind of like statements but not necessary
  // return a pointer when success, nullptr otherwise
  Type *buildTypeQualification();
  void addStructDefinition();

  // building the function decl
  ASTBase *buildFunctionDecl();
  FunctionArgLists *buildFunctionArgList();

  // Statements
  ASTBase *buildAssignmentStatement();
  ASTBase *buildReturnStatement();
  ASTBase *buildStatement();
  ASTBase *buildIfStatement();
  ASTBase *buildWhileStatement();
  ASTBase *buildDeclarationStatement();

  // Expressions
  ASTBase *buildExpression();
  ASTBase *buildBinaryExpression(int min_precedence);
  ASTBase *buildTrivialExpression();
  ASTBase *buildCallExpr();

  // FIXME: there can be a lot of cleanup.
  // In fact this entire thinh could be parsed without recursion
  RefYieldExpression *buildPosfixExpression(RefYieldExpression *lhs = nullptr);
  RefYieldExpression *
  buildTailPosfixExpression(RefYieldExpression *lhs); // helper for above

  inline ASTBase *logError(const char *message);

  // for binary expression
  const static inline std::unordered_map<lex::TokenType, int> precedence_level =
      {
          // eq, ne, gt, ge, lt, le
          {lex::EqualKeyword, 1}, {lex::NEquals, 1},  {lex::GreaterThan, 1},
          {lex::GreaterEqual, 1}, {lex::LessThan, 1}, {lex::LessEqual, 1},

          {lex::Add, 2},          {lex::Subtract, 2},

          {lex::Multiply, 3},
  };

  ContextHolder m_context;
  lex::Tokenizer m_tokenizer;
  Sema m_actions;

  bool m_error = false;

  // Store the computation results
  std::vector<ASTBase *> m_function_decls;
  std::unordered_map<std::string, StructType *> m_struct_defs;
};

#endif
