#include "type.h"
#include <llvm/IR/DerivedTypes.h>

bool Type::isBuiltin() const {
  return dynamic_cast<const BuiltinType *>(this) != nullptr;
}

bool Type::isStruct() const {
  return dynamic_cast<const StructType *>(this) != nullptr;
}

llvm::Type *Type::getType(ContextHolder holder) {
  assert(false && "please implement getType");
  return nullptr;
}

BuiltinType::BuiltinType(Builtin builtin)
    : m_builtin(builtin) {
        switch(m_builtin){
            case Int: 
                m_bits_size = 32;
                break;
            default: 
                assert(false && "how did we get here?");
        }
}

llvm::Type* BuiltinType::getType(ContextHolder holder){
    switch(m_builtin){
        case Int: 
            return llvm::Type::getIntNTy(holder->context, m_bits_size);
        default: 
            assert(false && "should be not possible");
            return nullptr;
    }
}

StructType::StructType(const std::vector<Element>& element,const std::string& name ):
    m_elements(element), m_name(name){

}

llvm::Type* StructType::getType(ContextHolder holder){
    if(m_llvm_type)
        return m_llvm_type;

    std::vector<llvm::Type*> elements {}; 
    for(Element ele : m_elements){
        Type* type = ele.type;
        llvm::Type* llvm_type = type->getType(holder);
        elements.push_back(llvm_type);
    }

    m_llvm_type =  llvm::StructType::create(elements);
    m_llvm_type->setName(m_name);
    return m_llvm_type;
}
