#ifndef AST_H
#define AST_H

#include <string>
#include <vector>

// llvm includes
#include "llvm/IR/Value.h"
#include "llvm/IR/Function.h"

#include "context.h"
class ASTBase{
public:
    virtual llvm::Value* codegen(ContextHolder holder); 
    virtual void dump();

private:
};

enum Type{
    Int32
};

struct TypeInfo{
    TypeInfo() = default;

    Type kind;
    std::string name; 
};

//============================== Miscellaneous ==============================

// FIXME: remove this class in the future. This makes
// this makes codegen have non trivial behavior!
// We are already expected a function declaration in the LLVM IR 
// level already, so what is the point of this class?
class FunctionArgLists : public ASTBase{
public:
    using ArgsIter = std::vector<TypeInfo>::const_iterator;

    FunctionArgLists(std::vector<TypeInfo>&& args);
    virtual llvm::Value* codegen(ContextHolder holder) override;

    ArgsIter begin() const;
    ArgsIter end() const;
private:
    std::vector<TypeInfo> m_args;
};

class FunctionDecl : public ASTBase{
public:
    FunctionDecl(std::vector<ASTBase*>&& expression, FunctionArgLists* arg_list, std::string&& name);

    virtual llvm::Value* codegen(ContextHolder holder) override;
    void dump() override; 
private:
    std::vector<ASTBase*> m_expressions;
    FunctionArgLists* m_arg_list;
    std::string m_name; 
};


//============================== Statements ==============================
enum AssignmentType{
    Constant, 
};

class AssignmentStatement : public ASTBase{
public: 
    AssignmentStatement(const std::string& name, long long value) ;

    const std::string& getName();
    long long getValue();
private:
    std::string m_name; 
    long long m_value; 
    AssignmentType m_type = Constant;
};

class ReturnStatement : public ASTBase{
public:
    // returning an identifier
    ReturnStatement(const std::string&name);

    virtual llvm::Value* codegen(ContextHolder holder) override;
private: 
    enum ReturnType{
        Identifier,
        Expression // Add this!
    };

    // Are we returning a value or a type?
    std::string m_name;
    ReturnType m_type;
};

#endif
