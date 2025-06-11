#include <cassert>
#include <iostream>
#include "ast.h"

void ASTBase::codegen(){
    std::cout << "not implemented" << std::endl;
    return; 
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
    return m_args.cbegin();
}

void FunctionDecl::codegen(){
    std::cout <<"I am generating code!" << std::endl;
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
