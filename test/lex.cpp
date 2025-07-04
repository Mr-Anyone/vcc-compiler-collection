#include "lex.h"
#include <cstdio>
#include <gtest/gtest.h>
#include <fstream>

// Demonstrate some basic assertions.
TEST(LexTest, TokenTest) {
    std::ofstream stream("/tmp/testing.txt");
    const char* file_input = "some_identifier function";
    stream << file_input;
    stream.close();

    lex::Tokenizer tokenizer ("/tmp/testing.txt");
    EXPECT_EQ(tokenizer.getCurrentType(), lex::Identifier);
    EXPECT_EQ(tokenizer.getNextType(), lex::FunctionDecl);

    std::remove("/tmp/testing");
}
