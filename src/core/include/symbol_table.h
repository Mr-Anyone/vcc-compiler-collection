#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <iostream>
#include <llvm/IR/Value.h>

class FunctionDecl;
class ASTBase;
class Type;

// Each TrieTrie insert and stores the following
struct CGTypeInfo {
  llvm::Value *value; // the allocated location on the stack
  Type *type;
};

/// There is only one TrieTree for every
/// FunctionDecl
///
/// Stores scope information used for
/// name lookup
/// function testing gives int [
///     int a,
///     int b,
/// ]{
///     if a eq b then
///         int c = 10;
///         ret a - b + c;
///     end
///     ret a + b;
/// }
/// FunctionDecl--(symbols a, b)
///      |-- If statement (symbols c)
class TrieTree {
public:
  TrieTree();
  TrieTree(const FunctionDecl *decl);

  /// insert a name with code gen value at a position pos
  void insert(const ASTBase *pos, std::string name, Type *type,
              llvm::Value *value);

  /// looking up a variable name named name at pos
  CGTypeInfo lookup(const ASTBase *pos, std::string name) const;

private:
  /// Given an empty array, put something like {FunctionDecl, IfStatement,
  /// WhileStatement} into trie_order
  void getTrieOrder(const ASTBase *start,
                    std::vector<const ASTBase *> &trie_order) const;

  struct TrieNode {
    // FIXME: maybe adding a static method for heap allocation is a good idea?
    // so then we can make this private
    TrieNode(const ASTBase *scope_decl);

    void dump();
    // name to value
    const ASTBase *scope_def; // must either be a scope specifier
    std::unordered_map<std::string, CGTypeInfo>
        decls; // data that is being stored
    std::unordered_map<const ASTBase *, std::shared_ptr<TrieNode>>
        child; // child[inner scope] = next;
  };
  using node_t = std::shared_ptr<TrieNode>;

  // The head of the TrieTree is not like a conventional. This
  // is always the function decl
  node_t head;
};

// Basically just like a hash table
class SymbolTable {
public:
  SymbolTable();

  void addFunction(const FunctionDecl *function_decl);
  const FunctionDecl *lookupFunction(const std::string &name);

  /// Adding a name named name at loc with value value
  void addLocalVariable(ASTBase *loc, std::string name, Type *type,
                        llvm::Value *value);
  // FIXME: it may be better to just return a struct that contains a bit
  // more type information
  CGTypeInfo lookupLocalVariable(ASTBase *at, std::string name);

private:
  std::unordered_map<std::string, TrieTree>
      m_local_variable_table; // m_local_variable_table[function name] = the
                              // corresponding TrieTree.
  std::unordered_map<std::string, const FunctionDecl *> m_function_table;
};

#endif
