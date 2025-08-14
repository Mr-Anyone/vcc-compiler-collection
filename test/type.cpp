#include "llvm/IR/Type.h"
#include "core/context.h"
#include "core/type.h"
#include <gtest/gtest.h>

TEST(Type, BasicTest) {
  vcc::ContextHolder holder =
      std::make_unique<vcc::GlobalContext>("resource/comp.txt");
  vcc::BuiltinType a(vcc::BuiltinType::BuiltinType::Int);
  vcc::BuiltinType b(vcc::BuiltinType::BuiltinType::Int);
  vcc::BuiltinType c(vcc::BuiltinType::BuiltinType::Int);

  EXPECT_TRUE(a.isBuiltin());
  EXPECT_FALSE(a.isStruct());
  EXPECT_EQ(a.getType(holder), llvm::Type::getIntNTy(holder->context, 32));

  // struct Outer{
  //     int a, int b, int c,
  // }
  vcc::StructType outer({{0, "a", &a}, {1, "b", &b}, {2, "c", &c}}, "asfdas");
  llvm::Type *interger = llvm::Type::getInt32Ty(holder->context);
  // the struct name for codegen has a struct prefix
  EXPECT_EQ(outer.getType(holder)->getStructName(), "struct.asfdas");
  EXPECT_EQ(outer.getElement("c").value().name, "c");
  EXPECT_EQ(outer.getElement("c").value().field_num, 2);
  EXPECT_EQ(outer.getElement("c").value().type->getType(holder),
            llvm::Type::getInt32Ty(holder->context));

  // Pointer test
  EXPECT_FALSE(outer.getElement("c").value().type->isPointer());

  vcc::PointerType pointer_to_a(&a);
  EXPECT_TRUE(pointer_to_a.isPointer());
  EXPECT_FALSE(pointer_to_a.isBuiltin());
  EXPECT_FALSE(pointer_to_a.isStruct());
  EXPECT_EQ(pointer_to_a.getPointee(), &a);
  EXPECT_EQ(
      pointer_to_a.getType(holder),
      llvm::PointerType::get(
          llvm::IntegerType::getInt32Ty(holder->context)->getContext(), 0));

  // array (10) array (20) int
  vcc::ArrayType base(&a, 20);
  vcc::ArrayType array(&base, 10);
  EXPECT_TRUE(base.isArray());
  EXPECT_FALSE(array.isStruct());
  EXPECT_FALSE(array.isBuiltin());
  EXPECT_FALSE(array.isPointer());
  EXPECT_TRUE(array.getBase()->isArray());
  EXPECT_TRUE(array.getBase()->getAs<vcc::ArrayType>()->getBase()->isBuiltin());
  EXPECT_EQ(
      array.getType(holder),
      llvm::ArrayType::get(
          llvm::ArrayType::get(llvm::Type::getInt32Ty(holder->context), 20),
          10));

  vcc::VoidType some_void_type;
  EXPECT_TRUE(some_void_type.isVoid());
  EXPECT_FALSE(some_void_type.isArray());
  EXPECT_FALSE(some_void_type.isBuiltin());
  EXPECT_FALSE(some_void_type.isStruct());
  EXPECT_EQ(llvm::Type::getVoidTy(holder->context),
            some_void_type.getType(holder));

  vcc::BuiltinType char_type(vcc::BuiltinType::Char);
  EXPECT_TRUE(char_type.isBuiltin());
  EXPECT_FALSE(char_type.isFloat());
  EXPECT_FALSE(char_type.isInt());
  EXPECT_FALSE(char_type.isArray());
  EXPECT_FALSE(char_type.isVoid());
  EXPECT_EQ(char_type.getType(holder), llvm::Type::getInt8Ty(holder->context));

  vcc::BuiltinType bool_type(vcc::BuiltinType::Bool);
  EXPECT_TRUE(bool_type.isBuiltin());
  EXPECT_FALSE(bool_type.isFloat());
  EXPECT_FALSE(bool_type.isInt());
  EXPECT_EQ(bool_type.getType(holder), llvm::Type::getInt1Ty(holder->context));
}
