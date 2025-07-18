#include <gtest/gtest.h>
#include "driver.h"
#include "parser.h"

TEST(CompTest, TestCompile){
    Parser parser = parseFile("resource/comp.txt");
    for(ASTBase* base : parser.getSyntaxTree()){
        base->codegen(parser.getHolder());
    }
    

    EXPECT_EQ(parser.haveError(), false);
}
