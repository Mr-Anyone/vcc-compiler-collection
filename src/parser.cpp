#include <cassert>
#include  <iostream>
#include "ast.h"
#include "parser.h"

Parser::Parser(const char* filename, ContextHolder context):
    m_tokenizer(filename), m_context(context){

}

ASTBase* Parser::buildSyntaxTree(){
    return buildFunctionDecl();
}


static ASTBase* logError(const char* message){
    std::cerr << "ERROR: " << message << "\n";
    return nullptr;
}

// function_decl :== 'function', <identifier>, 'gives', <type_qualification>,  
//                     <function_args_list>, '{', <expression>+, ''}'
ASTBase* Parser::buildFunctionDecl(){
    if(m_tokenizer.current().getType() != TokenType::FunctionDecl)
        return logError("function declaration must begin with keyword function");

    // eat function decl
    Token name_token = m_tokenizer.next();
    if(name_token.getType() != TokenType::Identifier)
        return logError("function declaration does not have identifier");

    std::string name = name_token.getStringLiteral();

    if(m_tokenizer.getNextType() != TokenType::Gives)
        return logError("function declaration must provide return type");

    // FIXME: it is possible to have other types
    // currently only int is supported
    assert(m_tokenizer.getNextType() == Int);
    FunctionArgLists* arg_list = buildFunctionArgList();

    if(m_tokenizer.getCurrentType() != TokenType::LeftBrace)
        return logError("expected {");
    m_tokenizer.consume();


    // currently only assignment expression is supported
    ASTBase* exp;
    std::vector<ASTBase*> expressions;
    while((exp = buildAssignmentStatement())){
        assert(exp && "expression must be non nullptr");
        expressions.push_back(exp);
    }

    return new class FunctionDecl (std::move(expressions)
            , dynamic_cast<FunctionArgLists*>(arg_list), std::move(name));
}

// assignment_expression :== <identifier>, '=', <integer_literal>, ';'
ASTBase* Parser::buildAssignmentStatement(){
    if(m_tokenizer.getCurrentType() != Identifier)
        return nullptr;

    assert(m_tokenizer.getCurrentType() == Identifier);
    std::string name = m_tokenizer.current().getStringLiteral();
    if(m_tokenizer.getNextType() != Equal)
        return nullptr;

    Token number_constant = m_tokenizer.next();
    if(number_constant.getType() != IntegerLiteral)
        return nullptr;

    long long value = number_constant.getIntegerLiteral();
    if(m_tokenizer.getNextType() != SemiColon)
        return nullptr; 
    m_tokenizer.consume();

    return  new AssignmentStatement (name, value);
}


// FIXME: maybe put arg_declaration into its own function?
// function_args_list :== '[', args_declaration+, ']'
//  args_declaration :== <type_qualification> + identifier + ','
FunctionArgLists* Parser::buildFunctionArgList(){
    if(m_tokenizer.getNextType() != LeftBracket){
        logError("expected [");
        return nullptr;
    }

    // parsing args declaration
    // FIXME: add a way to map token into type qualification

    std::vector<TypeInfo> args {};
    while(m_tokenizer.getNextType() == Int){
        // TokenType type_qualification = m_tokenizer.getCurrentType();
        
        Token next_token = m_tokenizer.next();
        if(next_token.getType() != Identifier){
            logError("expected [");
            return nullptr;
        }
        std::string name = next_token.getStringLiteral();

        if(m_tokenizer.getNextType() != Comma){
            logError("expected comma");
            return nullptr;
        }
        args.push_back(TypeInfo {Int32, name});
    }

    if(m_tokenizer.getCurrentType() != RightBracket){
        logError("expected ]");
        return nullptr;
    }
    
    // pop this token ]
    m_tokenizer.consume();

   return new FunctionArgLists (std::move(args));
}

ASTBase* Parser::buildReturnStatement(){
    if(m_tokenizer.getCurrentType() != Ret){
        return logError("expected error");
    }
    m_tokenizer.consume();

    if(m_tokenizer.getNextType() !=  Identifier){
        return logError("expected identifier");
    }

    std::string name = m_tokenizer.current().getStringLiteral();
    if(m_tokenizer.getNextType() != SemiColon){
        return logError("expected semi colon");
    }

    return new class ReturnStatement (name);
}
