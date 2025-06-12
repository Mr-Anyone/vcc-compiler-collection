#include <cassert>
#include <iostream>
#include <llvm/IR/DerivedTypes.h>
#include "ast.h"

llvm::Value* ASTBase::codegen(ContextHolder holder){
    assert(false && "the base code gen has been called!");
    return nullptr; 
}

void ASTBase::dump(){
    std::cout << "not implemented" << std::endl;
    return;
}

FunctionDecl::FunctionDecl(std::vector<ASTBase*>&& expression, FunctionArgLists* arg_list, std::string&& name): 
    ASTBase(), m_expressions(expression), m_arg_list(arg_list), m_name(name){

}

FunctionArgLists::FunctionArgLists(std::vector<TypeInfo>&& args):
    m_args(args){
}

FunctionArgLists::ArgsIter FunctionArgLists::begin() const {
    return m_args.cbegin();
}

FunctionArgLists::ArgsIter FunctionArgLists::end() const {
    return m_args.cend();
}

llvm::Value* FunctionDecl::codegen(ContextHolder holder){
    // FIXME: make this more efficient 
    std::vector<llvm::Type*> args; 
    std::vector<std::string> names;
    for(auto it = m_arg_list->begin(), ie = m_arg_list->end(); it != ie; ++it){
        args.push_back(llvm::Type::getInt32Ty(holder->context));
        names.push_back(it->name);
    }

    llvm::FunctionType* function_type = 
        llvm::FunctionType::get(llvm::Type::getInt32Ty(holder->context), args, /*isVarArg=*/false);

    llvm::Function* function = 
        llvm::Function::Create(function_type, llvm::Function::ExternalLinkage, m_name, holder->module);

    // set the name
    int count = 0;
    std::vector<llvm::Value*> values;
    for(llvm::Argument& arg: function->args()){
        arg.setName(names[count++]);
        values.push_back(&arg);
    }

    llvm::BasicBlock* block = llvm::BasicBlock::Create(holder->context, "func_start", function);
    holder->builder.SetInsertPoint(block);
    llvm::Value* ret_value = holder->builder.CreateAdd(values[0], values[1]);
    holder->builder.CreateRet(ret_value);

    return function;
}

AssignmentExpression::AssignmentExpression(const std::string& name, long long value): 
    ASTBase(), m_name(name), m_value(value) {
}

const std::string& AssignmentExpression::getName(){
    return m_name;
}

long long AssignmentExpression::getValue(){
    assert(m_type == Constant && "the value must be a constant");
    return m_value;
}

void FunctionDecl::dump(){
    std::cout << "function name: " << m_name << std::endl;
    std::cout << "args: " << m_arg_list << std::endl;
    m_arg_list->dump();

    std::cout << "dumping expressions:" << std::endl;

    for(auto* exp: m_expressions){
        exp->dump();
    }
}
