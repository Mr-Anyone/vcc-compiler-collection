#ifndef CORE_CONTEXT_H
#define CORE_CONTEXT_H

// the owner of everything
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>

#include <memory>

#include "core/stream.h"
#include "core/symbol_table.h"

namespace vcc
{
namespace lex
{
class Tokenizer;
struct Token;
};  // namespace lex

class DiagnosticDriver
{
   public:
    DiagnosticDriver(FileStream& stream);
    void diag(const std::string& message);

    /// Diagnose with a message at the current token of the tokenizer
    void diag(lex::Tokenizer& tokenizer, const std::string& message);
    void diag(const ASTBase* node, const std::string& message);

    /// True if there was as error being diagnose, a.k.a diag is Called
    bool hasError() const;

   private:
    void printFilePos(const FilePos& pos, const std::string& message);
    void printSeeHere(const FilePos& pos);

    void setError();

    /// True if diag was called!
    bool m_error = false;
    FileStream& m_stream;
};


class GlobalContext
{
   public:
    GlobalContext(const char* path_to_file);

    /// LLVM related context / variable
    llvm::IRBuilder<>& getBuilder();
    llvm::Module& getModule();
    llvm::LLVMContext& getContext();

    FileStream& getStream();
    LocalVariableTable& getLocalVariableTable();
    DiagnosticDriver& getDiagnosticsDriver();
    FunctionDeclTable& getFunctionDeclTable();

   private:
    llvm::LLVMContext m_context;
    llvm::IRBuilder<> m_builder;
    llvm::Module m_module;

    // symbol table
    LocalVariableTable m_symbol_table;
    FunctionDeclTable m_function_decl_table;
    DiagnosticDriver m_diag_driver;
    FileStream m_stream;
};

using ContextHolder = std::shared_ptr<GlobalContext>;
};  // namespace vcc
#endif
