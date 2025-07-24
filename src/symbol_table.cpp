#include "symbol_table.h"
#include "ast.h"
#include "util.h"

SymbolTable::SymbolTable() : m_local_variable_table(), m_function_table() {}

void SymbolTable::addFunction(const FunctionDecl *function_decl) {
  std::string function_name = function_decl->getName();
  assert(!m_function_table.contains(function_name));

  m_function_table[function_name] = function_decl;
}

const FunctionDecl *SymbolTable::lookupFunction(const std::string &name) {
  assert(m_function_table.contains(name));
  return m_function_table[name];
}

CGTypeInfo TrieTree::lookup(const ASTBase *at, std::string name) const {
  std::vector<const ASTBase *> trie_order;
  getTrieOrder(at, trie_order);

  // The trie searching order
  std::vector<node_t> search_order;
  search_order.push_back(head);
  node_t trie_at = head;
  for (int i = 1; i < trie_order.size(); ++i) {
    if (!trie_at->child.contains(trie_order[i]))
      break;

    trie_at = trie_at->child[trie_order[i]];
    // we are now at at next
    search_order.push_back(trie_at);
  }
  std::reverse(search_order.begin(), search_order.end());

  for (node_t node : search_order) {
    auto it = node->decls.find(name);
    // found what we are looking for
    if (it != node->decls.end())
      return it->second;
  }

  assert(false && "failed name lookup. undefined reference?");
  return {nullptr, nullptr};
}

TrieTree::TrieTree::TrieNode::TrieNode(const ASTBase *decl)
    : scope_def(decl), decls(), child() {
  assert(ASTBase::doesDefineScope(decl) && "decl must define a scope");
}

TrieTree::TrieTree(const FunctionDecl *decl)
    : head(std::make_unique<TrieNode>(decl)) {}

TrieTree::TrieTree() : head(nullptr) {}

void TrieTree::insert(const ASTBase *pos, std::string name, Type* type, llvm::Value *value) {
  std::vector<const ASTBase *> trie_insert_order;
  getTrieOrder(pos, trie_insert_order);

  // at the end of the iteration, it must be that
  node_t traverse_trie = head;
  for (int i = 1; i < trie_insert_order.size(); ++i) {
    const ASTBase *next_scope = trie_insert_order[i];
    if (traverse_trie->child.contains(next_scope)) {
      traverse_trie = traverse_trie->child[next_scope];
    } else {
      node_t next_node = std::make_unique<TrieNode>(next_scope);
      traverse_trie->child[next_scope] = next_node;
      traverse_trie = next_node;
    }
  }

  // finally adding the element into here
  assert(traverse_trie->scope_def == pos->getScopeDeclLoc() 
          && "we must be at the location of scope def when we are inserting");
  assert(!traverse_trie->decls.contains(name) && "duplicate definition?");
  traverse_trie->decls[name] = {value, type};
}

void TrieTree::getTrieOrder(const ASTBase *start,
                            std::vector<const ASTBase *> &trie_order) const {
  assert(start && "must be true" && trie_order.size() == 0 &&
         "must also be empty");
  start = start->getScopeDeclLoc();
  while (start) {
    trie_order.push_back(start);
    start = start->getScopeDeclLoc();
  }
  std::reverse(trie_order.begin(), trie_order.end());
}

void SymbolTable::addLocalVariable(ASTBase *loc, std::string name,
                                   Type *type, llvm::Value* value) {
  // Create a trie it does not exist
  if (!m_local_variable_table.contains(loc->getFirstFunctionDecl()->getName())) {
    TrieTree lookup_table(loc->getFirstFunctionDecl());
    m_local_variable_table[loc->getFirstFunctionDecl()->getName()] =
        lookup_table;
  }

  m_local_variable_table[loc->getFirstFunctionDecl()->getName()].insert(
      loc, name, type, value);
}

CGTypeInfo SymbolTable::lookupLocalVariable(ASTBase *at, std::string name) {
  std::string function_name = at->getFirstFunctionDecl()->getName();
  const TrieTree &trie = m_local_variable_table[function_name];
  return trie.lookup(at, name);
}

void TrieTree::TrieNode::dump(){
    for(auto it = decls.begin(), ie=decls.end(); it!=ie; ++it){
        std::cout << it->first << ",";
    }
    std::cout << "\n";

}
