#include "core/lex.h"
#include <assert.h>
#include <cstdlib>
#include <iostream>
#include <istream>
#include <string>

using namespace vcc::lex;

void Tokenizer::consume() { m_current_token = readOneToken(); }

Tokenizer::Tokenizer(FileStream &stream) : m_file(stream) {
  m_current_token = readOneToken();
}

const Token Tokenizer::next(int n) {
  int pos = m_file.tellg();

  assert(n >= 1 && "makes no sense otherwise");
  Token current = readOneToken();
  for (int i = 0; i < n - 1; ++i) {
    current = readOneToken();
  }

  m_file.seekg(pos);
  return current;
}

static bool is_valid_stoi(const std::string &str) {
  for (char c : str) {
    if (!std::isdigit(c))
      return false;
  }

  return true;
}

const Token Tokenizer::peek() {
  // saving the location
  long current = m_file.tellg();

  Token next_token = readOneToken();

  // restoring the location
  m_file.seekg(current);
  return next_token;
}

void Tokenizer::removeWhiteSpace() {
  if (!m_file.good())
    return;

  char peek;
  while ((peek = m_file.peek()) && !m_file.eof()) {
    // skip the entire line if we see a comment
    if (peek == '#') {
      char c = m_file.get();
      while (c != '\n') {
        c = m_file.get();
      }

      continue;
    }

    if (peek != ' ' && peek != '\n')
      break;

    // consume the thing character
    m_file.get();
  }
}

Token Tokenizer::readOneToken() {
  // put this into read one
  removeWhiteSpace();

  FilePos pos = m_file.getPos();
  if (m_file.eof()) {
    return Token(EndOfFile, pos);
  }

  // Important Invariant:
  // at the end of every iteartion  of the while loop
  // we must be at the end of the current keyword
  //  so something like this:  keyword\n
  //                                  ^ returns '\n' m_file.get() on the next
  //                                  loop
  std::string buf;
  bool is_first_time = true;
  while (!m_file.eof()) {
    char c;
    m_file.get(c);

    // if we see either of these, it means that we should stop reading token
    // because it doesn't make sense for the same token to be joined by ' ' or
    // \n
    if (c == ' ' || c == '\n' || c == 0) {
      break;
    }

    // we might be parsing a string, so we must check this "
    if (is_first_time && c == '"') {
      char current;
      do {
        current = m_file.get();
        if (current != '"') {
          buf += current;
        }
      } while (current != '"');
      return Token(String, buf, pos);
    }

    // we are parsing the general one character tokens like *, -, etc
    if (one_character_token.contains(c)) {
      if (is_first_time) {
        // creating the lookup term
        std::string lookup_term;
        lookup_term += c;
        // making the lookup term
        Token return_result(keyword_map.at(lookup_term), pos);
        return return_result;
      }

      m_file.seekg(m_file.tellg() - 1);
      break;
    }

    is_first_time = false;
    buf += c;
  }

  // this is a valid integer?
  if (is_valid_stoi(buf)) {
    long long result = std::stoll(buf);
    Token return_result(result, pos);
    return return_result;
  }

  // keywords function, gives, etc..
  if (isKeyword(buf)) {
    return Token(getKeyword(buf), pos);
  }

  // it must be an identifier than
  return Token(std::move(buf), pos);
}

bool Tokenizer::isKeyword(const std::string &keyword) {
  return keyword_map.contains(keyword);
}

const Token &Tokenizer::current() { return m_current_token; }

const Token Tokenizer::next() {
  m_current_token = readOneToken();

  return m_current_token;
}

TokenType Tokenizer::getKeyword(const std::string &keyword) {
  TokenType result = keyword_map.at(keyword);
  assert(TokenType::KeywordStart < result && TokenType::KeywordEnd > result &&
         "this is the invarient");

  return result;
}

Token::Token(long long number, FilePos pos)
    : type(IntegerLiteral), integer_literal(number), pos(pos) {}

const char *tokenTypeToString(TokenType type) {
  switch (type) {
  case IntegerLiteral:
    return "IntegerLiteral";
  case Identifier:
    return "Identifier";
  case EndOfFile:
    return "EndOfFile";
  case Invalid:
    return "Invalid";
  case KeywordStart:
    return "KeywordStart";
  case LeftParentheses:
    return "LeftParentheses";
  case RightParentheses:
    return "RightParentheses";
  case LeftBrace:
    return "LeftBrace";
  case RightBrace:
    return "RightBrace";
  case LeftBracket:
    return "LeftBracket";
  case RightBracket:
    return "RightBracket";
  case Comma:
    return "Comma";
  case FunctionDecl:
    return "FunctionDecl";
  case Gives:
    return "Gives";
  case SemiColon:
    return "SemiColon";
  case Equal:
    return "Equal";
  case Ret:
    return "Ret";
  case Int:
    return "Int";
  case Add:
    return "Add";
  case KeywordEnd:
    return "KeywordEnd";
  case Num:
    return "Num";
  case Multiply:
    return "Multilpy";
  default:
    return "Unknown TokenType";
  }
}

Token::Token() : type(Invalid) {}

void Token::dump() const {
  std::cout << "Token type: " << tokenTypeToString(type) << std::endl;
}

Token::Token(TokenType type, FilePos pos) : type(type), pos(pos) {
  assert((type == LeftParentheses || type == RightParentheses ||
          type == EndOfFile || (type > KeywordStart && type < KeywordEnd)) &&
         "it must be parenthesis type style or keyword!");
}

Token::Token(std::string &&string, FilePos pos)
    : type(Identifier), string_literal(string), pos(pos) {}

Token::Token(TokenType type, std::string &string, FilePos pos)
    : type(type), string_literal(string), pos(pos) {
  assert(type == Identifier || type == String);
}

const std::string &Token::getStringLiteral() const {
  assert(type == Identifier || type == String);
  return string_literal;
}

long long Token::getIntegerLiteral() const {
  assert(type == IntegerLiteral);
  return integer_literal;
}

bool Token::isBinaryOperator() const {
  return type > BinaryOperatorStart && type < BinaryOperatorEnd;
}

TokenType Token::getType() const { return type; }

TokenType Tokenizer::getNextType() { return next().getType(); }

TokenType Tokenizer::getCurrentType() { return m_current_token.getType(); }

vcc::FilePos Token::getPos() const { return pos; }

std::string Tokenizer::getLine(const FilePos &pos) {
  return m_file.getLine(pos.loc);
}

bool Token::isTypeQualification() const {
  return TypeQualificationStart < getType() && getType() < TypeQualificationEnd;
}

vcc::FilePos Tokenizer::getPos() { return current().getPos(); }
