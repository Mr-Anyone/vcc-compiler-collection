#ifndef LEX_H 
#define LEX_H

#include <fstream> 
#include <istream>
#include <set>
#include <unordered_map>

enum TokenType{
    IntegerLiteral, // 1234
    Identifier, // names 
    EndOfFile,
    Invalid, // used for error checking, and depends on context


    // have no string literal and integer literal
    // Keywords 
    KeywordStart,
    LeftParentheses, // (
    RightParentheses, // )
    LeftBrace, // {
    RightBrace, // }
    LeftBracket, // [
    RightBracket, // ]
    Comma,  // ,
    FunctionDecl,  // 'function'
    Gives, 
    SemiColon, //;
    Equal, // = 
    Ret, // ret

    // Type qualifications
    Int,

    KeywordEnd,

    // binary 
    BinarySymbol, // +, -, *, /

    // End 
    Num
};


enum BinaryOperation{
    Add, 
    // Subtract, 
    // Multiply, 
    // Divide, 
};

struct Token{
public:
    Token();
    // for identifier
    Token(std::string&& string_litearl);

    // for IntegerLiteral
    Token(long long integer_litearl);

    // set binary operation
    Token(BinaryOperation binary_operation);

    // for type like Parentheses, Invalid, EndOfFile, and keywords
    Token(TokenType type);

    void dump() const;
    BinaryOperation getBinaryOperation () const;
    TokenType getType() const;
    const std::string& getStringLiteral() const;
    long long getIntegerLiteral() const;
private:
    TokenType type;

    BinaryOperation op;
    std::string string_literal; 
    // FIXME: change this into int64_t 
    long long integer_literal;
};


class Tokenizer{
public:
    Tokenizer(const char* filename);

    // consume token
    const Token next();
    // don't consume token
    const Token& peek();
    const Token& current();

    // consume the token
    void consume();
    TokenType getNextType();
    TokenType getCurrentType();
private:
    const static inline std::unordered_map<std::string, TokenType> keyword_map = {
        {"function", FunctionDecl},
        {"(", LeftParentheses}, 
        {")", RightParentheses}, 
        {"[", LeftBracket},
        {"]", RightBracket},
        {"{", LeftBrace},
        {"}", RightBrace},
        {",", Comma},
        {"int", Int}, 
        {"gives", Gives},
        {";", SemiColon},
        {"=", Equal}
    };

    const static inline std::set<char> binary_operator  {'+'};
    static bool isKeyword(const std::string& keyword);
    static bool isBinary(const std::string& keyword);
    static TokenType getKeyword(const std::string&keyword);

    Token readOneToken();
    void removeWhiteSpace();
    std::ifstream m_file;

    Token m_current_token;
};


#endif
