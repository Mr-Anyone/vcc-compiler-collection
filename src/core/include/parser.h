#ifndef PARSER_H
#define PARSER_H

#include "ast.h"
#include "context.h"
#include "lex.h"
#include "sema.h"

#include "type.h"

class Parser {
public:
  Parser(ContextHolder context);

  void start();
  const std::vector<Statement *> &getSyntaxTree();
  ContextHolder getHolder();
  bool haveError() const;

private:
  const std::vector<Statement *> &buildSyntaxTree();

  // Types, kind of like statements but not necessary
  // return a pointer when success, nullptr otherwise
  Type *buildTypeQualification();
  void addStructDefinition();

  // building the function decl
  Statement *buildFunctionDecl();
  FunctionArgLists *buildFunctionArgList();

  // Statements
  Statement *buildAssignmentStatement();
  Statement *buildReturnStatement();
  Statement *buildStatement();
  Statement *buildIfStatement();
  Statement *buildWhileStatement();
  Statement *buildDeclarationStatement();
  Statement *buildExternalDecl();
  Statement *buildCallStatement();

  // Expressions
  Expression *buildExpression();
  Expression *buildRefExpression();
  Expression *buildCastExpression();
  Expression *buildBinaryExpression(int min_precedence);
  Expression *buildTrivialExpression();
  Expression *buildDerefExpression();
  Expression *buildCallExpr();
  LocatorExpression *buildPosfixExpression(LocatorExpression *lhs = nullptr);
  LocatorExpression *
  buildTailPosfixExpression(LocatorExpression *lhs); // helper for above

  /// This is a way so that we can use one interface to log all the error
  /// The problem we are trying to solve is that logError have to return a type
  /// that is compatible to Expression , Type*, and Statement*.
  struct ErrorResult {
    inline operator Expression *() { return nullptr; }
    inline operator Statement *() { return nullptr; }
    inline operator Type *() { return nullptr; }
  };
  inline ErrorResult logError(const std::string &message);

  // for binary expression
  // clang-format off
  const static inline std::unordered_map<lex::TokenType, int> precedence_level={
          // eq, ne, gt, ge, lt, le
          {lex::EqualKeyword, 1},
          {lex::NEquals, 1},
          {lex::GreaterThan, 1},
          {lex::GreaterEqual, 1},
          {lex::LessThan, 1},
          {lex::LessEqual, 1},

          {lex::Add, 2},
          {lex::Subtract, 2},

          {lex::Multiply, 3},
          {lex::Divide, 3},
  };
  // clang-format on

  ContextHolder m_context;
  lex::Tokenizer m_tokenizer;
  Sema m_actions;

  // Store the computation results
  std::vector<Statement *> m_top_level_statements;
  std::unordered_map<std::string, StructType *> m_struct_defs;
};

#endif
