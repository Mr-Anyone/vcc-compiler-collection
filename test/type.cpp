#include "llvm/IR/Type.h"
#include "context.h"
#include "type.h"
#include <gtest/gtest.h>

TEST(Type, BasicTest){
    ContextHolder holder = std::make_unique<GlobalContext>();
    BuiltinType a(BuiltinType::BuiltinType::Int);
    BuiltinType b(BuiltinType::BuiltinType::Int);
    BuiltinType c(BuiltinType::BuiltinType::Int);

    EXPECT_TRUE(a.isBuiltin());
    EXPECT_FALSE(a.isStruct());
    EXPECT_EQ(a.getType(holder), llvm::Type::getIntNTy(holder->context, 32));

    // struct Outer{
    //     int a, int b, int c,
    // }
    StructType outer ({{"a", &a}, {"b", &b}, {"c", &c}}, "asfdas"); 
    llvm::Type* interger = llvm::Type::getInt32Ty(holder->context);
    EXPECT_EQ(outer.getType(holder)->getStructName(), "asfdas");
}
