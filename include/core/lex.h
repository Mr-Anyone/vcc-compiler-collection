#ifndef CORE_LEX_H
#define CORE_LEX_H

#include "core/stream.h"
#include <set>
#include <unordered_map>
#include <unordered_set>

namespace vcc {
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
  Fullstop,         // .
  LessSign,         // <
  GreaterSign,      // >
  Gives,
  External,  // external
  Deref,     // deref
  Ref,       // deref
  SemiColon, //;
  Equal,     // =
  Ret,       // ret
  Cast,      // cast
  If,        // If
  Then,      // Then
  End,       // End
  While,     // while
  String,    //  for "+.+", so like "some text here"

  // Type qualifications
  TypeQualificationStart,
  Int,    // int
  Struct, // struct
  Array,  // array
  Ptr,    // ptr
  Char,   // char
  Float,  // float
  Bool,   // bool
  Long,   // long
  Short,  // short
  TypeQualificationEnd,

  BinaryOperatorStart, // Binary operator start
  Void,                // float
  Add,                 // +
  Subtract,            // -
  Multiply,            // *
  Divide,              // /
  EqualKeyword,        // eq
  NEquals,             // ne
  GreaterThan,         // gt
  GreaterEqual,        // ge
  LessThan,            // lt
  LessEqual,           // le
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

  // for type like Parentheses, Invalid, EndOfFile, and keywords
  Token(TokenType type, FilePos pos);

  Token(TokenType type, std::string &string_literal, FilePos pos);

  void dump() const;
  TokenType getType() const;
  const std::string &getStringLiteral() const;
  long long getIntegerLiteral() const;
  bool isBinaryOperator() const;
  FilePos getPos() const;
  bool isTypeQualification() const;

private:
  FilePos pos = {1, 1, 0};
  TokenType type;

  std::string string_literal;
  // FIXME: change this into int64_t
  long long integer_literal;
};

class Tokenizer {
public:
  Tokenizer(FileStream &stream);

  // consume token
  const Token next();
  const Token next(int n);

  // don't consume token
  const Token peek();
  const Token &current();

  // consume the token
  void consume();
  TokenType getNextType();
  TokenType getCurrentType();
  FilePos getPos();

  std::string getLine(const FilePos &pos);

private:
  const static inline std::unordered_map<std::string, TokenType> keyword_map = {
      {"function", FunctionDecl},
      {"(", LeftParentheses},
      {")", RightParentheses},
      {"<", LessSign},
      {">", GreaterSign},
      {"[", LeftBracket},
      {"]", RightBracket},
      {"{", LeftBrace},
      {"}", RightBrace},
      {",", Comma},
      {"while", While},
      {"struct", Struct},
      {"external", External},

      {"if", If},
      {"then", Then},
      {"end", End},
      {"deref", Deref},
      {"ref", Ref},

      // boolean stuff
      {"eq", EqualKeyword},
      {"ne", NEquals},
      {"gt", GreaterThan},
      {"ge", GreaterEqual},
      {"le", LessEqual},
      {"lt", LessThan},

      // binary stuff

      {"int", Int},
      {"float", Float},
      {"long", Long},
      {"short", Short},
      {"array", Array},
      {"bool", Bool},
      {"gives", Gives},
      {"cast", Cast},
      {"char", Char},
      {"void", Void},
      {"ptr", Ptr},
      {";", SemiColon},
      {"=", Equal},
      {".", Fullstop},
      {"ret", Ret},
      {"+", Add},
      {"/", Divide},
      {"-", Subtract},
      {"*", Multiply}};

  static std::unordered_set<char>
  getOneCharacterToken(const std::unordered_map<std::string, TokenType> &map) {
    std::unordered_set<char> one_symbol{};
    for (auto it = map.begin(), ie = map.end(); it != ie; ++it) {
      if (it->first.length() == 1)
        one_symbol.insert(it->first[0]);
    }
    return one_symbol;
  }

  const static inline std::unordered_set<char> one_character_token =
      getOneCharacterToken(keyword_map);
  const static inline std::set<char> binary_operator{'+'};
  static bool isKeyword(const std::string &keyword);
  static bool isBinary(const std::string &keyword);
  static TokenType getKeyword(const std::string &keyword);

  Token readOneToken();
  void removeWhiteSpace();

  /// FIXME: this really is a trust me, I am always valid lifetime
  FileStream &m_file;

  Token m_current_token;
};
}; // namespace lex
}; // namespace vcc

#endif
