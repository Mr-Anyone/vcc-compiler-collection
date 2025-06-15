#include <llvm/IR/Constant.h>
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
    ASTBase(), m_statements(expression), m_arg_list(arg_list), m_name(name){

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

void FunctionArgLists::codegen(ContextHolder holder, llvm::Function* func){
    // appending to symbol table
    int count = 0;
    for(llvm::Argument& arg: func->args()){
        const std::string& name = m_args[count++].name;
        arg.setName(name);  

        // allocating one integer
        llvm::Value* alloc_loc =  
            holder->builder.CreateAlloca(llvm::Type::getInt32Ty(holder->context));
        holder->builder.CreateStore(&arg, alloc_loc);

        assert(holder->symbol_table.find(name) == holder->symbol_table.end()&& "cannot have multilpe definition");
        holder->symbol_table[name] = alloc_loc;
    }
}

llvm::Value* FunctionDecl::codegen(ContextHolder holder){
    // FIXME: make this more efficient 
    std::vector<llvm::Type*> args; 
    for(auto it = m_arg_list->begin(), ie=m_arg_list->end(); it != ie; ++it){
        args.push_back(llvm::Type::getInt32Ty(holder->context));
    }
    
 llvm::FunctionType* function_type = 
        llvm::FunctionType::get(llvm::Type::getInt32Ty(holder->context), args, /*isVarArg=*/false);

    llvm::Function* function = 
        llvm::Function::Create(function_type, llvm::Function::ExternalLinkage, m_name, holder->module);

    // generating code for something
    llvm::BasicBlock* block = llvm::BasicBlock::Create(holder->context, "main", function);
    holder->builder.SetInsertPoint(block);

    // copy the parameter into llvm ir
    m_arg_list->codegen(holder, function);

    // code generation for statement
    for(ASTBase*statement: m_statements){
        statement->codegen(holder);
    }

    return function;
}

AssignmentStatement::AssignmentStatement(const std::string& name, long long value): 
    ASTBase(), m_name(name), m_value(value) {
    }

llvm::Value* AssignmentStatement::codegen(ContextHolder holder){
    llvm::Value* alloc_loc = holder->symbol_table[m_name];
    switch(m_type){
        case Constant: 
             holder->builder.CreateStore(llvm::ConstantInt::get(llvm::Type::getInt32Ty(holder->context), getValue()), alloc_loc);
            break; 
        default: 
            assert(false&& "how did I made it here?");

    }

    return nullptr;
}

const std::string& AssignmentStatement::getName(){
    return m_name;
}

long long AssignmentStatement::getValue(){
    assert(m_type == Constant && "the value must be a constant");
    return m_value;
}

void FunctionDecl::dump(){
    std::cout << "function name: " << m_name << std::endl;
    std::cout << "args: " << m_arg_list << std::endl;
    std::cout << "args is not implemented for now!" << std::endl;

    std::cout << "dumping expressions:" << std::endl;

    for(auto* exp: m_statements){
        exp->dump();
    }
}

ReturnStatement::ReturnStatement(const std::string& name): 
    ASTBase(), m_name(name), m_type(ReturnType::Identifier){
    }

llvm::Value* ReturnStatement::codegen(ContextHolder holder){
    std::cout << "I've been called?" << std::endl;

    // TODO: it seems that more information needs to be encoded the symbol table
    assert(holder->symbol_table.find(m_name) != holder->symbol_table.end()
            && "must have an entry!");

    // TODO: we need to encode more type information!
    llvm::Value* ret_val = holder->builder.CreateLoad(llvm::Type::getInt32Ty(holder->context)
            , holder->symbol_table[m_name]);
    holder->builder.CreateRet(ret_val);

    return nullptr;
}
