#include "lex.h"
#include <assert.h>
#include <iostream>
#include <istream>
#include <string>

using namespace lex;

void Tokenizer::consume() { m_current_token = readOneToken(); }

Tokenizer::Tokenizer(const char *filename) : m_file(filename) {
  // FIXME: handle this better!
  if (!m_file.is_open()) {
    std::cerr << "cannot open file:" << filename << "\n";
    std::exit(-1);
  }

  m_current_token = readOneToken();
}

static bool is_valid_stoi(const std::string &str) {
  for (char c : str) {
    if (!std::isdigit(c))
      return false;
  }

  return true;
}

void Tokenizer::removeWhiteSpace() {
  if (!m_file.good())
    return;

  char c;
  while ((c = m_file.peek())) {
    if (!(c == ' ' || c == '\n')) {
      break;
    }

    // consume the thing character
    m_file.get();
  }
}

Token Tokenizer::readOneToken() {
  // put this into read one
  removeWhiteSpace();

  if (!m_file.good() && m_file.peek() == std::char_traits<char>::eof()) {
    return Token(EndOfFile);
  }

  std::string buf;
  char c;
  bool is_first_time = true;
  while (m_file.get(c)) {
    if (c == ' ' || c == '\n') {
      break;
    }

    // consume if this is the first time
    if (c == '(' || c == ')' || c == '[' || c == ']' || c == '{' || c == '}' ||
        c == ',' || c == '+' || c == ';' || c == '=') {
      // this is a binary operator
      if (is_first_time && binary_operator.find(c) != binary_operator.end()) {
        return Token(BinaryOperation::Add);
      }

      if (is_first_time) {
        // creating the lookup term
        std::string lookup_term;
        lookup_term += c;
        // making the lookup term
        Token return_result(keyword_map.at(lookup_term));
        return return_result;
      }

      m_file.seekg(m_file.tellg() - std::streampos(1));
      break;
    }

    is_first_time = false;
    buf += c;
  }

  // this is a valid integer?
  if (is_valid_stoi(buf)) {
    long long result = std::stoll(buf);
    Token return_result(result);
    return return_result;
  }

  // like FunctionDecl, etc
  if (isKeyword(buf)) {
    return Token(getKeyword(buf));
  }

  // it must be an identifier than
  return Token(std::move(buf));
}

bool Tokenizer::isKeyword(const std::string &keyword) {
  return keyword_map.find(keyword) != keyword_map.end();
}

const Token &Tokenizer::current() { return m_current_token; }

const Token Tokenizer::next() {
  m_current_token = readOneToken();

  return m_current_token;
}
const Token &Tokenizer::peek() {
  assert(false && "implement this somehow");

  return m_current_token;
}

TokenType Tokenizer::getKeyword(const std::string &keyword) {
  TokenType result = keyword_map.at(keyword);
  assert(TokenType::KeywordStart < result && TokenType::KeywordEnd > result &&
         "this is the invarient");

  return result;
}

Token::Token(long long number)
    : type(IntegerLiteral), integer_literal(number) {}

void Token::dump() const {
  switch (type) {
  case Gives:
    std::cout << "token (gives): gives" << std::endl;
    ;
    break;
  case IntegerLiteral:
    std::cout << "token (integer literal): " << integer_literal << std::endl;
    ;
    break;
  case Invalid:
    std::cout << "token (invalid): "
              << "invalid" << std::endl;
    break;
  case LeftParentheses:
    std::cout << "token (left parentheses): (" << std::endl;
    break;
  case RightParentheses:
    std::cout << "token (right parentheses): )" << std::endl;
    break;
  case EndOfFile:
    std::cout << "token (end of file): end of file" << std::endl;
    break;
  case FunctionDecl:
    std::cout << "token (function decl): function decl" << std::endl;
    break;
  case Identifier:
    std::cout << "token (identifier): " << string_literal << std::endl;
    break;
  case Int:
    std::cout << "token (int): int" << std::endl;
    break;
  case LeftBracket:
    std::cout << "token ([): [" << std::endl;
    break;

  case RightBracket:
    std::cout << "token (]): ]" << std::endl;
    break;
  case Comma:
    std::cout << "token (,): ," << std::endl;
    break;
  case LeftBrace:
    std::cout << "token ({): {" << std::endl;
    break;
  case RightBrace:
    std::cout << "token (}): }" << std::endl;
    break;
    // this is not correct
  case BinarySymbol:
    std::cout << "token (binary operation): +" << std::endl;
    break;
  case SemiColon:
    std::cout << "token (semicolon): ;" << std::endl;
    break;
  default:
    assert(false && "don't know how to dump this token");
  }
}

Token::Token() : type(Invalid) {}

Token::Token(TokenType type) : type(type) {
  assert((type == LeftParentheses || type == RightParentheses ||
          type == EndOfFile || (type > KeywordStart && type < KeywordEnd)) &&
         "it must be parenthesis type style or keyword!");
}

Token::Token(std::string &&string) : type(Identifier), string_literal(string) {}

const std::string &Token::getStringLiteral() const {
  assert(type == Identifier);
  return string_literal;
}

long long Token::getIntegerLiteral() const {
  assert(type == IntegerLiteral);
  return integer_literal;
}

TokenType Token::getType() const { return type; }

Token::Token(BinaryOperation operation) : type(BinarySymbol), op(operation) {}

BinaryOperation Token::getBinaryOperation() const { return op; }

TokenType Tokenizer::getNextType() { return next().getType(); }

TokenType Tokenizer::getCurrentType() { return m_current_token.getType(); }
