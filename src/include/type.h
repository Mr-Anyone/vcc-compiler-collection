#ifndef TYPE_H
#define TYPE_H

#include "context.h"
#include <llvm/IR/Type.h>
#include <memory.h> 


class Type;
using type_ptr_t = std::shared_ptr<Type>;

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
};


class StructType : public Type {
public:
  struct Element{
    std::string name;
    Type* type;
  };
  StructType(const std::vector<Element>& elements, const std::string& name);
  virtual llvm::Type *getType(ContextHolder holder) override;
private:
  std::vector<Element> m_elements;
  std::string m_name; 
  llvm::StructType* m_llvm_type = nullptr;
};

#endif
