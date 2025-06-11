#ifndef AST_H
#define AST_H

#include <string>
#include <vector>

class ASTBase{
public:
    virtual void codegen(); 
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

class FunctionArgLists : public ASTBase{
public:
    using ArgsIter = std::vector<TypeInfo>::const_iterator;

    FunctionArgLists(std::vector<TypeInfo>&& args);

    ArgsIter begin() const;
    ArgsIter end() const;
private:
    std::vector<TypeInfo> m_args;
};

class FunctionDecl : public ASTBase{
public:
    FunctionDecl(std::vector<ASTBase*>&& expression, FunctionArgLists* arg_list, std::string&& name);

    virtual void codegen() override;
    void dump() override; 
private:
    std::vector<ASTBase*> m_expressions;
    FunctionArgLists* m_arg_list;
    std::string m_name; 
};

enum AssignmentType{
    Constant, 
};

class AssignmentExpression : public ASTBase{
public: 
    AssignmentExpression(const std::string& name, long long value) ;

    const std::string& getName();
    long long getValue();
private:
    std::string m_name; 
    long long m_value; 
    AssignmentType m_type = Constant;
};

#endif
