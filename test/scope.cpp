#include "core/scope.h"

#include <gtest/gtest.h>

using namespace vcc;

TEST(ScopeTest, BasicTestOne)
{
    Scope* root         = Scope::createScope(ScopeType::TranslationUnit);
    Scope* firstChild   = Scope::createScope(ScopeType::IfStatement, root);
    Scope* childOfFirst = Scope::createScope(ScopeType::WhileStatement, firstChild);
    Scope* secondChild  = Scope::createScope(ScopeType::WhileStatement, root);

    // Test one: check to see if it dumps correctly
    // clang-format: off
    std::string expectedResult =
        "TranslationUnit\n"
        "|---| IfStatement\n"
        "|---|---| WhileStatement\n"
        "|---| WhileStatement\n";
    // clang-format: on

    std::string output;
    root->dump(output);

    EXPECT_EQ(output, expectedResult);

    // Test two: check for parents
    EXPECT_EQ(root->parent(), nullptr);
    EXPECT_EQ(firstChild->parent(), root);
    EXPECT_EQ(secondChild->parent(), root);
    EXPECT_EQ(childOfFirst->parent(), firstChild);

    // Test three: making sure all of the child exist in the parent
    auto existInVec = []<typename T>(const std::vector<T>& vec, T target)
    {
        for (T ele : vec)
        {
            if (ele == target)
            {
                return true;
            }
        }
        return false;
    };
    EXPECT_EQ(root->children().size(), 2);
    EXPECT_TRUE(existInVec(root->children(), firstChild));
    EXPECT_TRUE(existInVec(root->children(), secondChild));
    EXPECT_EQ(firstChild->children().size(), 1);
    EXPECT_TRUE(existInVec(firstChild->children(), childOfFirst));
    EXPECT_EQ(childOfFirst->children().size(), 0);
    EXPECT_EQ(secondChild->children().size(), 0);
}
