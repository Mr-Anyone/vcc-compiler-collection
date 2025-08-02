#include "lex.h"
#include <cstdio>
#include <fstream>
#include <gtest/gtest.h>

// Demonstrate some basic assertions.
TEST(LexTest, TokenTest) {
  std::ofstream stream("/tmp/testing.txt");
  const char *file_input = "some_identifier function +";
  stream << file_input;
  stream.close();

  vcc::FileStream some_stream("/tmp/testing.txt");
  vcc::lex::Tokenizer tokenizer(some_stream);
  EXPECT_EQ(tokenizer.getCurrentType(), vcc::lex::Identifier);
  EXPECT_EQ(tokenizer.getNextType(), vcc::lex::FunctionDecl);
  EXPECT_EQ(tokenizer.getNextType(), vcc::lex::Add);

  std::remove("/tmp/testing.txt");
}

TEST(LexTest, EndOfFileReading) {
  vcc::FileStream some_stream("resource/lex_end_of_file.vcc");
  vcc::lex::Tokenizer tokenizer(some_stream);
  while (tokenizer.getCurrentType() != vcc::lex::EndOfFile) {
    tokenizer.next();
  }

  // testing multiple times to see if this works
  EXPECT_EQ(vcc::lex::EndOfFile, tokenizer.getNextType());
  EXPECT_EQ(vcc::lex::EndOfFile, tokenizer.getNextType());
  EXPECT_EQ(vcc::lex::EndOfFile, tokenizer.getNextType());
  EXPECT_EQ(vcc::lex::EndOfFile, tokenizer.getNextType());
}

TEST(LexTest, NewToken) {
  std::ofstream stream("/tmp/testing2.txt");
  const char *file_input =
      "some_identifier function + eq ne gt ge le lt "
      "if then end - while struct . array ptr float void external"
      "< > ref  char \"This is Literally A Test\""
      "\n bool";
  stream << file_input;
  stream.close();

  vcc::FileStream stream_two("/tmp/testing2.txt");
  vcc::lex::Tokenizer tokenizer(stream_two);
  EXPECT_EQ(tokenizer.getCurrentType(), vcc::lex::Identifier);
  EXPECT_EQ(tokenizer.getNextType(), vcc::lex::FunctionDecl);
  EXPECT_EQ(tokenizer.getNextType(), vcc::lex::Add);
  EXPECT_EQ(tokenizer.getNextType(), vcc::lex::EqualKeyword);
  EXPECT_EQ(tokenizer.getNextType(), vcc::lex::NEquals);
  EXPECT_EQ(tokenizer.getNextType(), vcc::lex::GreaterThan);
  EXPECT_EQ(tokenizer.getNextType(), vcc::lex::GreaterEqual);
  EXPECT_EQ(tokenizer.getNextType(), vcc::lex::LessEqual);
  EXPECT_EQ(tokenizer.getNextType(), vcc::lex::LessThan);
  EXPECT_EQ(tokenizer.getNextType(), vcc::lex::If);
  EXPECT_EQ(tokenizer.getNextType(), vcc::lex::Then);
  EXPECT_EQ(tokenizer.getNextType(), vcc::lex::End);
  EXPECT_EQ(tokenizer.getNextType(), vcc::lex::Subtract);
  EXPECT_EQ(tokenizer.getNextType(), vcc::lex::While);
  EXPECT_EQ(tokenizer.getNextType(), vcc::lex::Struct);
  EXPECT_EQ(tokenizer.getNextType(), vcc::lex::Fullstop);
  EXPECT_EQ(tokenizer.getNextType(), vcc::lex::Array);
  EXPECT_EQ(tokenizer.getNextType(), vcc::lex::Ptr);
  EXPECT_EQ(tokenizer.getNextType(), vcc::lex::Float);
  EXPECT_EQ(tokenizer.getNextType(), vcc::lex::Void);
  EXPECT_EQ(tokenizer.getNextType(), vcc::lex::External);
  EXPECT_EQ(tokenizer.getNextType(), vcc::lex::LessSign);
  EXPECT_EQ(tokenizer.getNextType(), vcc::lex::GreaterSign);
  EXPECT_EQ(tokenizer.getNextType(), vcc::lex::Ref);
  EXPECT_EQ(tokenizer.getNextType(), vcc::lex::Char);
  EXPECT_EQ(tokenizer.getNextType(), vcc::lex::String);
  EXPECT_EQ(tokenizer.current().getStringLiteral(), "This is Literally A Test");
  EXPECT_EQ(tokenizer.getNextType(), vcc::lex::Bool);

  std::remove("/tmp/testing2.txt");
}
