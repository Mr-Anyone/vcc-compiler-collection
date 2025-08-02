#ifndef CONTEXT_H
#define CONTEXT_H

// the owner of everything
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>

#include "symbol_table.h"
#include <memory>

namespace lex {
class Tokenizer;
struct Token;
}; // namespace lex

#include "stream.h"

class DiagnosticDriver {
public:
  void diag(const std::string &message);
  /// FIXME: this is terrible style, maybe we should just pass the line to be
  /// printed, or just a format. This is because there could be
  /// mutability form tokenizer
  ///
  /// Diagnose with a message at the current token of the tokenizer
  void diag(lex::Tokenizer &tokenizer, const std::string &message);
  void diag(const ASTBase *node, const std::string &line_in_file,
            const std::string &message);

  /// True if there was as error being diagnose, a.k.a diag is Called
  bool hasError() const;

private:
  void printFilePos(const FilePos &pos, const std::string &message);
  void printSeeHere(const FilePos &pos);

  void setError();
  /// True if diag was called!
  bool m_error = false;
};

// FIXME: this really should be a class
struct GlobalContext {
  GlobalContext(const char *path_to_file);

  llvm::LLVMContext context;
  llvm::IRBuilder<> builder;
  llvm::Module module;

  // symbol table
  SymbolTable symbol_table;
  DiagnosticDriver diagnostics;
  FileStream stream;

  inline std::string getLine(const FilePos& pos){ return stream.getLine(pos.loc);}
};

using ContextHolder = std::shared_ptr<GlobalContext>;

#endif
