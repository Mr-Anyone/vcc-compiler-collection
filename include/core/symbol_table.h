#ifndef CORE_SYMBOL_TABLE_H
#define CORE_SYMBOL_TABLE_H

#include <llvm/IR/Value.h>

#include <iostream>

#include "adt/optional.h"
#include "core/ast_base.h"
#include "core/scope.h"

namespace vcc
{
class FunctionDecl;
class Type;

struct CGTypeInfo
{
    llvm::Value* value;
    Type* type;
};

struct SymbolTableKey
{
    Scope* scope;
    std::string name;

    inline bool operator==(const SymbolTableKey& other) const
    {
        return other.scope == this->scope && other.name == this->name;
    }
};

template <typename InsertType>
class SymbolTable
{
   public:
    SymbolTable() {}
    SymbolTable(const SymbolTable& other) = delete;

    /// Returns true on success, false otherwise
    bool insert(ASTBase* loc, std::string name, InsertType value)
    {
        return insert(loc->getScope(), name, value);
    }

    /// Returns true on success, false otherwise
    bool insert(Scope* loc, std::string name, InsertType value)
    {
        SymbolTableKey key{.scope = loc, .name = name};
        if (m_table.find(key) != m_table.end())
        {
            return false;
        }

        m_table[key] = value;
        return true;
    }

    /// exist traverse the entire scope tree to find if a
    /// ancestor has a name
    void exist(ASTBase* loc, std::string name)
    {
        return exist(loc->getScope(), name);
    }

    void exist(Scope* loc, std::string name)
    {
        return !lookup(loc, name).isEmtpy();
    }

    Optional<InsertType> lookup(ASTBase* at, std::string name)
    {
        return lookup(at->getScope(), name);
    }

    Optional<InsertType> lookup(Scope* at, std::string name)
    {
        // Lookup is a bit more complicated since we have to
        while (at)
        {
            SymbolTableKey key{.scope = at, .name = name};

            if (m_table.find(key) != m_table.end())
            {
                return Optional(m_table[key]);
            }

            at = at->parent();
        }

        return Optional<InsertType>();
    }

   private:
    std::unordered_map<SymbolTableKey, InsertType> m_table;
};

using LocalVariableTable = SymbolTable<Type*>;
using FunctionDeclTable  = SymbolTable<FunctionDecl*>;
};  // namespace vcc

// provide hash function to the symbol table
template <>
struct std::hash<vcc::SymbolTableKey>
{
    std::size_t operator()(const vcc::SymbolTableKey& k) const
    {
        return hash<vcc::Scope*>{}(k.scope) ^ (hash<std::string>{}(k.name) << 1);
    }
};
// provide local variable to the hash table
#endif
