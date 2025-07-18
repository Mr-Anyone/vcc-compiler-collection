#include "type.h"
#include "util.h"

bool Type::isBuiltin() const {
  return dyncast<const BuiltinType *>(this) != nullptr;
}

bool Type::isStruct() const {
  return dyncast<const StructType *>(this) != nullptr;
}

llvm::Type *Type::getType(ContextHolder holder) {
  assert(false && "please implement getType");
  return nullptr;
}

BuiltinType::BuiltinType(Builtin builtin, int bits_size)
    : m_builtin(builtin), m_bits_size(bits_size) {
    }
