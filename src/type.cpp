#include "type.h"
#include <llvm/IR/DerivedTypes.h>

bool Type::isBuiltin() const {
  return dynamic_cast<const BuiltinType *>(this) != nullptr;
}

bool Type::isStruct() const {
  return dynamic_cast<const StructType *>(this) != nullptr;
}

bool Type::isPointer() const {
  return dynamic_cast<const PointerType *>(this) != nullptr;
}

bool Type::isArray() const {
  return dynamic_cast<const ArrayType *>(this) != nullptr;
}

llvm::Type *Type::getType(ContextHolder holder) {
  assert(false && "please implement getType");
  return nullptr;
}

BuiltinType::BuiltinType(Builtin builtin) : m_builtin(builtin) {
  switch (m_builtin) {
  case Float:
  case Int:
    m_bits_size = 32;
    break;
  default:
    assert(false && "how did we get here?");
  }
}

BuiltinType::Builtin BuiltinType::getKind() const { return m_builtin; }

llvm::Type *BuiltinType::getType(ContextHolder holder) {
  if (m_llvm_type)
    return m_llvm_type;

  switch (m_builtin) {
  case Int:
    m_llvm_type = llvm::Type::getIntNTy(holder->context, m_bits_size);
    return m_llvm_type;
  case Float:
    m_llvm_type = llvm::Type::getFloatTy(holder->context);
    return m_llvm_type;
  default:
    assert(false && "should be not possible");
    return nullptr;
  }
}

StructType::StructType(const std::vector<Element> &element,
                       const std::string &name)
    : m_elements(element), m_name(name) {
#ifndef NDEBUG
  for (int i = 0; i < m_elements.size(); ++i) {
    assert(m_elements[i].field_num == i &&
           "The array makes no sense otherwise");
  }
#endif
}

llvm::Type *StructType::getType(ContextHolder holder) {
  if (m_llvm_type)
    return m_llvm_type;

  std::vector<llvm::Type *> elements{};
  for (Element ele : m_elements) {
    Type *type = ele.type;
    llvm::Type *llvm_type = type->getType(holder);
    elements.push_back(llvm_type);
  }

  m_llvm_type = llvm::StructType::create(elements);
  m_llvm_type->setName("struct." + m_name);
  return m_llvm_type;
}

// maybe we should use a string instead?
std::optional<StructType::Element>
StructType::getElement(const std::string &name) {
  for (const Element &element : m_elements) {
    if (element.name == name)
      return std::make_optional<Element>(element);
  }
  return std::nullopt;
}

PointerType::PointerType(Type *pointee) : m_pointee(pointee) {}

Type *PointerType::getPointee() { return m_pointee; }

llvm::Type *PointerType::getType(ContextHolder holder) {
  return llvm::PointerType::get(m_pointee->getType(holder), /*AddressSpace=*/0);
}

ArrayType::ArrayType(Type *base, int count) : m_count(count), m_base(base) {}

Type *ArrayType::getBase() { return m_base; }

int ArrayType::getCount() { return m_count; }

llvm::Type *ArrayType::getType(ContextHolder holder) {
  if (m_llvm_type)
    return m_llvm_type;

  return llvm::ArrayType::get(m_base->getType(holder), m_count);
}

