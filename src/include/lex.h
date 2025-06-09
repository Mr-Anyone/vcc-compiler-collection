#ifndef LEX_H 
#define LEX_H

#include <fstream> 

enum TokenType{
    StringLiteral, 
    NumberLiteral, 
    Keyword, 
    Num
};

struct Token{
    Token();

    TokenType type;
};


class Tokenizer{
public:
    Tokenizer();

    // consume token
    Token next();

    // don't consume token
    Token peak();

    Token current();
};

class Lexer{
public:
    Lexer(const char* filename);
    void start();

private:
    std::ifstream filename; 
};


#endif
