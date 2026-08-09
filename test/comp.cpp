#include <gtest/gtest.h>

#include "core/codegen.h"
#include "core/driver.h"
#include "core/parser.h"

TEST(CompTest, TestCompile)
{
    vcc::Parser parser = vcc::parseFile("resource/comp.vcc");
    vcc::CodeGenerator codegen(parser.getHolder());
    for (vcc::Statement* base : parser.getSyntaxTree())
    {
        codegen.emitStatement(base);
    }

    EXPECT_EQ(parser.haveError(), false);
}
