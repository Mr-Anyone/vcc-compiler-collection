#ifndef LEX_H
#define LEX_H

#include "stream.h"
#include <fstream>
#include <istream>
#include <set>
#include <unordered_map>

namespace lex {

enum TokenType {
  IntegerLiteral, // 1234
  Identifier,     // names
  EndOfFile,
  Invalid, // used for error checking, and depends on context

  // have no string literal and integer literal
  // Keywords
  KeywordStart,
  LeftParentheses,  // (
  RightParentheses, // )
  LeftBrace,        // {
  RightBrace,       // }
  LeftBracket,      // [
  RightBracket,     // ]
  Comma,            // ,
  FunctionDecl,     // 'function'
  Gives,
  SemiColon, //;
  Equal,     // =
  Ret,       // ret

  // Type qualifications
  Int,

  BinaryOperatorStart, // Binary operator start
  Add,                 // +
  Multiply,            // *
  BinaryOperatorEnd,   // Binary operator ends

  KeywordEnd,

  // End
  Num
};

struct Token {
public:
    // error state
  Token();

  // for identifier
  Token(std::string &&string_litearl, FilePos pos);

  // for IntegerLiteral
  Token(long long integer_litearl, FilePos pos);

  // set binary operation

  // for type like Parentheses, Invalid, EndOfFile, and keywords
  Token(TokenType type, FilePos pos);

  void dump() const;
  TokenType getType() const;
  const std::string &getStringLiteral() const;
  long long getIntegerLiteral() const;
  bool isBinaryOperator() const;
  FilePos getPos() const;

private:
  FilePos pos = {1, 1, 0};
  TokenType type;

  std::string string_literal;
  // FIXME: change this into int64_t
  long long integer_literal;
};

class Tokenizer {
public:
  Tokenizer(const char *filename);

  // consume token
  const Token next();
  // don't consume token
  const Token peek();
  const Token &current();

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
      {"=", Equal},
      {"ret", Ret},
      {"+", Add},
      {"*", Multiply}};

  const static inline std::set<char> binary_operator{'+'};
  static bool isKeyword(const std::string &keyword);
  static bool isBinary(const std::string &keyword);
  static TokenType getKeyword(const std::string &keyword);

  Token readOneToken();
  void removeWhiteSpace();
  FileStream m_file;

  Token m_current_token;
};
}; // namespace lex

#endif
