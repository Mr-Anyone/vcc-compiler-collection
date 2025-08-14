#include "core/type.h"
#include <llvm/IR/DerivedTypes.h>
#include <string_view>

using namespace vcc;

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

bool Type::isVoid() const {
  return dynamic_cast<const VoidType *>(this) != nullptr;
}

bool Type::isVoidPtr() const {
  if (!isPointer())
    return false;

  return dyncast<const PointerType>(this)->getPointee()->isVoid();
}

llvm::Type *Type::getType(ContextHolder holder) {
  assert(false && "please implement getType");
  return nullptr;
}

BuiltinType::BuiltinType(Builtin builtin) : m_builtin(builtin) {
  switch (m_builtin) {
  case Bool:
    m_bits_size = 1;
    break;
  case Char:
    m_bits_size = 8;
    break;
  case Short:
    m_bits_size = 16;
    break;
  case Float:
  case Int:
    m_bits_size = 32;
    break;
  case Long:
    m_bits_size = 64;
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
  case Long:
  case Short:
  case Bool:
  case Char:
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
#ifdef NDEBUG
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
const Type *PointerType::getPointee() const { return m_pointee; }

llvm::Type *PointerType::getType(ContextHolder holder) {
  return llvm::PointerType::get(m_pointee->getType(holder)->getContext(),
                                /*AddressSpace*/ 0);
}

ArrayType::ArrayType(Type *base, int count) : m_count(count), m_base(base) {}

Type *ArrayType::getBase() { return m_base; }

int ArrayType::getCount() { return m_count; }

llvm::Type *ArrayType::getType(ContextHolder holder) {
  if (m_llvm_type)
    return m_llvm_type;

  return llvm::ArrayType::get(m_base->getType(holder), m_count);
}

void Type::dump() { std::cout << "unknown type"; }

void ArrayType::dump() {
  std::cout << "array (" << m_count << ") ";
  getBase()->dump();
}

void PointerType::dump() {
  std::cout << "ptr ";
  getPointee()->dump();
}

void BuiltinType::dump() {
  switch (m_builtin) {
  case Int:
    std::cout << "int";
    break;
  case Float:
    std::cout << "float";
    break;
  default:
    assert(false && "don't know how to print type");
  }
}

void StructType::dump() {
  std::cout << "struct { ";
  for (Element &ele : m_elements) {
    ele.type->dump();
    std::cout << "|";
  }

  std::cout << " }";
}

bool BuiltinType::isInt() const { return m_builtin == Int; }

bool BuiltinType::isFloat() const { return m_builtin == Float; }

const std::vector<StructType::Element> &StructType::getElements() const {
  return m_elements;
}

const std::string &StructType::getName() const { return m_name; }

bool Type::isSame(Type *lhs, Type *rhs) {
  if (lhs->isStruct() && rhs->isStruct()) {
    StructType *the_lhs = lhs->getAs<StructType>();
    StructType *the_rhs = rhs->getAs<StructType>();
    if (the_lhs->getName() != the_rhs->getName())
      return false;

    auto rhs_elements = the_rhs->getElements();
    auto lhs_elements = the_lhs->getElements();

    if (lhs_elements.size() != rhs_elements.size())
      return false;

    for (int i = 0; i < lhs_elements.size(); ++i) {
      assert(lhs_elements[i].field_num == rhs_elements[i].field_num &&
             "invariant check");
      if (lhs_elements[i].name != rhs_elements[i].name)
        return false;

      if (!isSame(lhs_elements[i].type, rhs_elements[i].type)) {
        return false;
      }
    }

    return true;
  }

  if (lhs->isBuiltin() && rhs->isBuiltin()) {
    return lhs->getAs<BuiltinType>()->getKind() ==
           rhs->getAs<BuiltinType>()->getKind();
  }

  if (lhs->isPointer() && rhs->isPointer()) {
    return isSame(lhs->getAs<PointerType>()->getPointee(),
                  rhs->getAs<PointerType>()->getPointee());
  }

  if (lhs->isArray() && rhs->isArray()) {
    if (lhs->getAs<ArrayType>()->getCount() !=
        rhs->getAs<ArrayType>()->getCount())
      return false;

    return isSame(lhs->getAs<ArrayType>()->getBase(),
                  rhs->getAs<ArrayType>()->getBase());
  }

  if (lhs->isVoid() && rhs->isVoid())
    return true;

  return false;
}

llvm::Type *VoidType::getType(ContextHolder holder) {
  return llvm::Type::getVoidTy(holder->context);
}

void VoidType::dump() { std::cout << "void"; }

bool BuiltinType::isBool() const { return m_builtin == Bool; }

bool BuiltinType::isChar() const { return m_builtin == Char; }

bool BuiltinType::isShort() const { return m_builtin == Short; }

bool BuiltinType::isLong() const { return m_builtin == Long; }

bool BuiltinType::isIntegerKind() const {
  return isBool() || isChar() || isInt() || isShort() || isLong();
}

int BuiltinType::getBitSize() const { return m_bits_size; }
