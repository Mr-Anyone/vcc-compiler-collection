#include "llvm/IR/Type.h"
#include "context.h"
#include "type.h"
#include <gtest/gtest.h>

TEST(Type, BasicTest) {
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
  StructType outer({{0, "a", &a}, {1, "b", &b}, {2, "c", &c}}, "asfdas");
  llvm::Type *interger = llvm::Type::getInt32Ty(holder->context);
  // the struct name for codegen has a struct prefix
  EXPECT_EQ(outer.getType(holder)->getStructName(), "struct.asfdas");
  EXPECT_EQ(outer.getElement("c").value().name, "c");
  EXPECT_EQ(outer.getElement("c").value().field_num, 2);
  EXPECT_EQ(outer.getElement("c").value().type->getType(holder),
            llvm::Type::getInt32Ty(holder->context));

  // Pointer test
  EXPECT_FALSE(outer.getElement("c").value().type->isPointer());

  PointerType pointer_to_a(&a);
  EXPECT_TRUE(pointer_to_a.isPointer());
  EXPECT_FALSE(pointer_to_a.isBuiltin());
  EXPECT_FALSE(pointer_to_a.isStruct());
  EXPECT_EQ(pointer_to_a.getPointee(), &a);
  EXPECT_EQ(pointer_to_a.getType(holder),
            llvm::Type::getInt32PtrTy(holder->context));

  // array (10) array (20) int
  ArrayType base(&a, 20);
  ArrayType array(&base, 10);
  EXPECT_TRUE(base.isArray());
  EXPECT_FALSE(array.isStruct());
  EXPECT_FALSE(array.isBuiltin());
  EXPECT_FALSE(array.isPointer());
  EXPECT_TRUE(array.getBase()->isArray());
  EXPECT_TRUE(array.getBase()->getAs<ArrayType>()->getBase()->isBuiltin());
  EXPECT_EQ(
      array.getType(holder),
      llvm::ArrayType::get(
          llvm::ArrayType::get(llvm::Type::getInt32Ty(holder->context), 20),
          10));
}
