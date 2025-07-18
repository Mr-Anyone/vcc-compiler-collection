#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <iostream>
#include <llvm/IR/Value.h>

class FunctionDecl;
class ASTBase;

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
//// FunctionDecl--(symbols a, b)
////      |-- If statement (symbols c)
class TrieTree {
public:
    TrieTree();
  TrieTree(FunctionDecl* decl);

  /// insert a name with code gen value at a position pos
  void insert(ASTBase *pos, std::string name, llvm::Value *value = nullptr);

  /// looking up a variable name named name at pos
  llvm::Value *lookup(ASTBase* pos, std::string name) const;

private:
  /// Given an empty array, put something like {FunctionDecl, IfStatement, WhileStatement} 
  /// into trie_order
  void getTrieOrder(ASTBase* start, std::vector<ASTBase*>& trie_order) const;

  struct TrieNode {
      // FIXME: maybe adding a static method for heap allocation is a good idea?
      // so then we can make this private
      TrieNode(ASTBase* scope_decl);

      void dump();
    // name to value
    ASTBase *scope_def; // must either be a scope specifier
    std::unordered_map<std::string, llvm::Value *> decls; // data that is being stored
    std::unordered_map<ASTBase*, std::shared_ptr<TrieNode>> child; // child[inner scope] = next;
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

  void addFunction(FunctionDecl *function_decl);
  llvm::Function *lookupFunction(const std::string &name);

  /// Adding a name named name at loc with value value 
  void addLocalVariable(ASTBase *loc, std::string name,
                        llvm::Value *value);
  // FIXME: it may be better to just return a struct that contains a bit
  // more type information
  llvm::Value *lookupLocalVariable(ASTBase *at, std::string name);

private:
  std::unordered_map<std::string, TrieTree>
      m_local_variable_table; // m_local_variable_table[function name] = the
                              // corresponding TrieTree.
  std::unordered_map<std::string, llvm::Function *> m_function_table;
};

#endif
