#include "core/driver.h"
#include "core/parser.h"

#include <gtest/gtest.h>

TEST(CompTest, TestCompile) {
  vcc::Parser parser = vcc::parseFile("resource/comp.vcc");
  for (vcc::Statement *base : parser.getSyntaxTree()) {
    base->codegen(parser.getHolder());
  }

  EXPECT_EQ(parser.haveError(), false);
}
