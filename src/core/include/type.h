#ifndef TYPE_H
#define TYPE_H

#include "context.h"
#include <llvm/IR/Type.h>
#include <memory.h>
#include <optional>

#include "util.h"

// FIXME: A lot of the time Type is immutable
// Therefore maybe we could just return a pointer
// by heap allocating once and save a lot of save.
// we might need something like a map to keep track
class Type {
public:
  virtual llvm::Type *getType(ContextHolder holder);
  virtual void dump();

  template <typename T> T *getAs() { return dyncast<T>(this); }

  bool isStruct() const;
  bool isBuiltin() const;
  bool isPointer() const;
  bool isArray() const;
  bool isVoid() const;

  static bool isSame(Type *lhs, Type *rhs);

private:
};

class ArrayType : public Type {
public:
  // Creating an array of base*, with these amount of count
  ArrayType(Type *base, int count);

  Type *getBase();
  int getCount();

  virtual llvm::Type *getType(ContextHolder holder) override;
  virtual void dump() override;

private:
  llvm::Type *m_llvm_type = nullptr;
  int m_count;
  Type *m_base;
};

class PointerType : public Type {
public:
  PointerType(Type *m_pointee);

  Type *getPointee();
  virtual llvm::Type *getType(ContextHolder holder) override;
  virtual void dump() override;

private:
  Type *m_pointee;
};

class BuiltinType : public Type {
public:
  enum Builtin { Int, Float, Char, Bool };

  BuiltinType(Builtin builtin);
  Builtin getKind() const;
  bool isFloat() const;

  bool isInt() const;
  bool isChar() const;
  bool isBool() const;

  /// isIntegerKind - True if the type is a int, char, or bool 
  bool isIntegerKind() const;

  int getBitSize() const;


  virtual llvm::Type *getType(ContextHolder holder) override;

  virtual void dump() override;

private:
  Builtin m_builtin;
  int m_bits_size;
  llvm::Type *m_llvm_type = nullptr;
};

class StructType : public Type {
public:
  struct Element {
    // FIXME: this can be deduced from array index. Why do we need this?
    int field_num;
    std::string name;
    Type *type;
  };
  StructType(const std::vector<Element> &elements, const std::string &name);
  virtual llvm::Type *getType(ContextHolder holder) override;
  virtual void dump() override;

  std::optional<Element> getElement(const std::string &name);
  const std::vector<Element> &getElements() const;
  const std::string &getName() const;

private:
  std::vector<Element> m_elements;
  std::string m_name;
  llvm::StructType *m_llvm_type = nullptr;
};

class VoidType : public Type {
public:
  virtual llvm::Type *getType(ContextHolder holder);
  virtual void dump();

private:
};

/// FIXME: it seems that we need to better organize
/// header files in the future!
/// used by ast.h implementation
struct TypeInfo {
  Type *type;
  std::string name;
};
#endif
