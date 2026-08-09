#ifndef CORE_SCOPE_H
#define CORE_SCOPE_H

#include <memory>
#include <string>
#include <vector>

#include "core/lex.h"

namespace vcc
{
enum class ScopeType
{
    TranslationUnit,
    FunctionDecl,
    WhileStatement,
    IfStatement
};

/// Scope is a nice class for parsing since it allows us
/// to add symbol into the symbol table while in the middle of
/// parsing. The construction of ASTBase* is only valid at the
/// final stage of parsing.
/// What scope can look like:
/// TranslationUnit
///  |
///  |
///  | - FunctionDecl:
///       |
///       | - WhileStatement
///       |    |
///       |    |
///       |    | - IfStatement
///       |
///       | - If Statement
/// ....
class Scope
{
   public:
    /// Create scope for the child.
    static Scope* createScope(ScopeType type, Scope* parent = nullptr);
    static Scope* createScope(lex::TokenType type, Scope* parent = nullptr);

    ScopeType getScope();
    void dump();
    void dump(std::string& buffer);

    const std::vector<Scope*>& children();
    Scope* parent() const;
    Scope* getTopMostScope();

   private:
    void addChild(Scope* child);
    void dumpWithDepth(int depth, std::string& buffer);

    // Scope should only be heap allocated so we are putting
    // this inside a private class
    Scope(ScopeType currentType, Scope* parent);

    ScopeType m_type;
    std::vector<Scope*> m_children;
    Scope* m_parent;
};
};  // namespace vcc

#endif
