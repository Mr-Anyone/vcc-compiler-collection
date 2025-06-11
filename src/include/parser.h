#ifndef PARSER_H
#define PARSER_H

#include "lex.h"
#include "ast.h"
#include <istream>

class Parser{
public:
    Parser(const char* filename);

    // nullptr if failed
    ASTBase* buildSyntaxTree();
private:
    // building the function decl
    ASTBase* buildFunctionDecl();
    ASTBase* buildFunctionArgList();
    ASTBase* buildAssignmentExpression();

    ASTBase* nextTokenOrError(TokenType expected_token, const char* message);

    Tokenizer m_tokenizer;
};

#endif
