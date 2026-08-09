#include "core/scope.h"

#include <algorithm>
#include <iostream>

#include "core/util.h"

using namespace vcc;

Scope* Scope::createScope(ScopeType type, Scope* parent)
{
    return new Scope{type, parent};
}

Scope::Scope(ScopeType type, Scope* parent) : m_type(type), m_children(), m_parent(parent)
{
    if (m_parent)
    {
        m_parent->addChild(this);
    }
}

Scope* Scope::parent() const
{
    return m_parent;
}

const std::vector<Scope*>& Scope::children()
{
    return m_children;
}

void Scope::dump()
{
    std::string buffer;
    dumpWithDepth(0, buffer);
    std::cout << buffer;
}

void Scope::dump(std::string& buffer)
{
    dumpWithDepth(0, buffer);
}

void Scope::dumpWithDepth(int depth, std::string& buffer)
{
    auto codeToString = [](ScopeType type) -> std::string
    {
        switch (type)
        {
            case ScopeType::TranslationUnit:
                return "TranslationUnit";
            case ScopeType::WhileStatement:
                return "WhileStatement";
            case ScopeType::IfStatement:
                return "IfStatement";
            case ScopeType::FunctionDecl:
                return "FunctionDecl";
            default:
                return "UnknownScopeType";
        }
    };

    auto getDepth = [](int depth) -> std::string
    {
        if (depth == 0)
            return "";

        std::string result = "|";
        for (int i = 0; i < depth; ++i)
        {
            result += "---|";
        }
        result += " ";

        return result;
    };


    buffer += getDepth(depth) + codeToString(getScope()) + "\n";
    for (Scope* children : children())
        children->dumpWithDepth(depth + 1, buffer);
}

void Scope::addChild(Scope* child)
{
    m_children.push_back(child);
}

ScopeType Scope::getScope()
{
    return m_type;
}

Scope* Scope::createScope(lex::TokenType type, Scope* parent)
{
    auto lexTypeToScopeType = [&](lex::TokenType type) -> ScopeType
    {
        switch (type)
        {
            case lex::TokenType::FunctionDecl:
                return ScopeType::FunctionDecl;
            case lex::TokenType::If:
                return ScopeType::IfStatement;
            case lex::TokenType::While:
                return ScopeType::WhileStatement;
            default:
                VCC_UNREACHABLE("this should crash here!");
        }
    };

    return createScope(lexTypeToScopeType(type), parent);
}

Scope* Scope::getTopMostScope()
{
    Scope* current = this;
    while (current->parent())
    {
        current = current->parent();
    }

    return current;
}
