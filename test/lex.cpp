#include "lex.h"
#include <cstdio>
#include <gtest/gtest.h>
#include <fstream>

// Demonstrate some basic assertions.
TEST(LexTest, TokenTest) {
    std::ofstream stream("/tmp/testing.txt");
    const char* file_input = "some_identifier function +";
    stream << file_input;
    stream.close();

    lex::Tokenizer tokenizer ("/tmp/testing.txt");
    EXPECT_EQ(tokenizer.getCurrentType(), lex::Identifier);
    EXPECT_EQ(tokenizer.getNextType(), lex::FunctionDecl);
    EXPECT_EQ(tokenizer.getNextType(), lex::Add);

    std::remove("/tmp/testing");
}

TEST(LexTest, EndOfFileReading){
    lex::Tokenizer tokenizer("resource/lex_end_of_file.txt");
    while(tokenizer.getCurrentType() != lex::EndOfFile){
        tokenizer.next();
    }

    // testing multiple times to see if this works 
    EXPECT_EQ(lex::EndOfFile, tokenizer.getNextType());
    EXPECT_EQ(lex::EndOfFile, tokenizer.getNextType());
    EXPECT_EQ(lex::EndOfFile, tokenizer.getNextType());
    EXPECT_EQ(lex::EndOfFile, tokenizer.getNextType());
}
