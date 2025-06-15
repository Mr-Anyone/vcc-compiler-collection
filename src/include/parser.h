#ifndef PARSER_H
#define PARSER_H

#include "lex.h"
#include "ast.h"
#include "context.h"

#include <istream>

class Parser{
public:
    Parser(const char* filename, ContextHolder context);

    // nullptr if failed
    ASTBase* buildSyntaxTree();
private:
    // building the function decl
    ASTBase* buildFunctionDecl();
    FunctionArgLists* buildFunctionArgList();

    ASTBase* buildAssignmentStatement();
    ASTBase* buildReturnStatement();
    ASTBase* buildStatement();

    ASTBase* nextTokenOrError(TokenType expected_token, const char* message);

    ContextHolder m_context;
    Tokenizer m_tokenizer;
};

#endif
