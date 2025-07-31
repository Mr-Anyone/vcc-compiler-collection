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

  FileStream some_stream("/tmp/testing.txt");
  lex::Tokenizer tokenizer(some_stream);
  EXPECT_EQ(tokenizer.getCurrentType(), lex::Identifier);
  EXPECT_EQ(tokenizer.getNextType(), lex::FunctionDecl);
  EXPECT_EQ(tokenizer.getNextType(), lex::Add);

  std::remove("/tmp/testing.txt");
}

TEST(LexTest, EndOfFileReading) {
  FileStream some_stream("resource/lex_end_of_file.txt");
  lex::Tokenizer tokenizer(some_stream);
  while (tokenizer.getCurrentType() != lex::EndOfFile) {
    tokenizer.next();
  }

  // testing multiple times to see if this works
  EXPECT_EQ(lex::EndOfFile, tokenizer.getNextType());
  EXPECT_EQ(lex::EndOfFile, tokenizer.getNextType());
  EXPECT_EQ(lex::EndOfFile, tokenizer.getNextType());
  EXPECT_EQ(lex::EndOfFile, tokenizer.getNextType());
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

  FileStream stream_two("/tmp/testing2.txt");
  lex::Tokenizer tokenizer(stream_two);
  EXPECT_EQ(tokenizer.getCurrentType(), lex::Identifier);
  EXPECT_EQ(tokenizer.getNextType(), lex::FunctionDecl);
  EXPECT_EQ(tokenizer.getNextType(), lex::Add);
  EXPECT_EQ(tokenizer.getNextType(), lex::EqualKeyword);
  EXPECT_EQ(tokenizer.getNextType(), lex::NEquals);
  EXPECT_EQ(tokenizer.getNextType(), lex::GreaterThan);
  EXPECT_EQ(tokenizer.getNextType(), lex::GreaterEqual);
  EXPECT_EQ(tokenizer.getNextType(), lex::LessEqual);
  EXPECT_EQ(tokenizer.getNextType(), lex::LessThan);
  EXPECT_EQ(tokenizer.getNextType(), lex::If);
  EXPECT_EQ(tokenizer.getNextType(), lex::Then);
  EXPECT_EQ(tokenizer.getNextType(), lex::End);
  EXPECT_EQ(tokenizer.getNextType(), lex::Subtract);
  EXPECT_EQ(tokenizer.getNextType(), lex::While);
  EXPECT_EQ(tokenizer.getNextType(), lex::Struct);
  EXPECT_EQ(tokenizer.getNextType(), lex::Fullstop);
  EXPECT_EQ(tokenizer.getNextType(), lex::Array);
  EXPECT_EQ(tokenizer.getNextType(), lex::Ptr);
  EXPECT_EQ(tokenizer.getNextType(), lex::Float);
  EXPECT_EQ(tokenizer.getNextType(), lex::Void);
  EXPECT_EQ(tokenizer.getNextType(), lex::External);
  EXPECT_EQ(tokenizer.getNextType(), lex::LessSign);
  EXPECT_EQ(tokenizer.getNextType(), lex::GreaterSign);
  EXPECT_EQ(tokenizer.getNextType(), lex::Ref);
  EXPECT_EQ(tokenizer.getNextType(), lex::Char);
  EXPECT_EQ(tokenizer.getNextType(), lex::String);
  EXPECT_EQ(tokenizer.current().getStringLiteral(), "This is Literally A Test");
  EXPECT_EQ(tokenizer.getNextType(), lex::Bool);

  std::remove("/tmp/testing2.txt");
}
