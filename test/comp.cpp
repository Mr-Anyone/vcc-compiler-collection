#include "driver.h"
#include "parser.h"
#include <gtest/gtest.h>

TEST(CompTest, TestCompile) {
  Parser parser = parseFile("resource/comp.txt");
  for (Statement *base : parser.getSyntaxTree()) {
    base->codegen(parser.getHolder());
  }

  EXPECT_EQ(parser.haveError(), false);
}
