#ifndef TYPE_H
#define TYPE_H

#include "context.h"
#include <llvm/IR/Type.h>
#include <optional>
#include <memory.h> 

// FIXME: A lot of the time Type is immutable 
// Therefore maybe we could just return a pointer 
// by heap allocating once and save a lot of save.
// we might need something like a map to keep track
class Type {
public:
  virtual llvm::Type *getType(ContextHolder holder);

  bool isStruct() const;
  bool isBuiltin() const;
private:
};

class BuiltinType : public Type {
public:
  enum Builtin {
    Int,
  };

  BuiltinType(Builtin builtin);
  virtual llvm::Type *getType(ContextHolder holder) override;

private:
  Builtin m_builtin;
  int m_bits_size; 
  llvm::Type* m_llvm_type = nullptr;
};


class StructType : public Type {
public:
  struct Element{
    // FIXME: this can be deduced from array index. Why do we need this?
    int field_num;
    std::string name;
    Type* type;
  };
  StructType(const std::vector<Element>& elements, const std::string& name);
  virtual llvm::Type *getType(ContextHolder holder) override;

  std::optional<Element> getElement(const std::string& name);
private:
  std::vector<Element> m_elements;
  std::string m_name; 
  llvm::StructType* m_llvm_type = nullptr;
};

#endif
