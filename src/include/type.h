#ifndef TYPE_H
#define TYPE_H

#include "context.h"
#include <llvm/IR/Type.h>

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

  BuiltinType(Builtin builtin, int bits_size);
  virtual llvm::Type *getType(ContextHolder holder) override;

private:
  Builtin m_builtin;
  int m_bits_size; 
};

class StructType : public Type {
public:
  StructType();
  virtual llvm::Type *getType(ContextHolder holder) override;

private:
  std::vector<Type> m_elements;
};

#endif
