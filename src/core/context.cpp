#include "core/context.h"

#include "core/ast.h"
#include "core/lex.h"
#include "core/stream.h"

using namespace vcc;

GlobalContext::GlobalContext(const char* path_to_file)
    : m_context(),
      m_builder(m_context),
      m_module("my module", m_context),
      m_symbol_table(),
      m_function_decl_table(),
      m_diag_driver(),
      m_stream(path_to_file)
{
}

llvm::IRBuilder<>& GlobalContext::getBuilder()
{
    return m_builder;
}

llvm::Module& GlobalContext::getModule()
{
    return m_module;
}

llvm::LLVMContext& GlobalContext::getContext()
{
    return m_context;
}

FileStream& GlobalContext::getStream()
{
    return m_stream;
}

LocalVariableTable& GlobalContext::getLocalVariableTable()
{
    return m_symbol_table;
}

FunctionDeclTable& GlobalContext::getFunctionDeclTable()
{
    return m_function_decl_table;
}

DiagnosticDriver& GlobalContext::getDiagnosticsDriver()
{
    return m_diag_driver;
}

/// ================= Start of DiagnosticDriver =================
///

void DiagnosticDriver::diag(const std::string& message)
{
    setError();

    std::cerr << message << std::endl;
}

void DiagnosticDriver::diag(lex::Tokenizer& tokenizer, const std::string& message)
{
    setError();
    printFilePos(tokenizer.getPos(), message);

    std::string line = tokenizer.getLine(tokenizer.getPos());
    std::cerr << line << "\n";
    printSeeHere(tokenizer.getPos());
}

void DiagnosticDriver::diag(const ASTBase* node, const std::string& line_in_file,
                            const std::string& message)
{
    setError();
    printFilePos(node->getPos(), message);
    std::cerr << line_in_file << "\n";
    printSeeHere(node->getPos());
}

void DiagnosticDriver::setError()
{
    m_error = true;
}

bool DiagnosticDriver::hasError() const
{
    return m_error;
}

void DiagnosticDriver::printFilePos(const FilePos& pos, const std::string& message)
{
    std::cerr << pos.row << ":" << pos.col << " Error: " << message << "\n";
}

void DiagnosticDriver::printSeeHere(const FilePos& pos)
{
    for (int i = 0; i < pos.col - 1; ++i)
    {
        std::cerr << " ";
    }

    std::cerr << "^---see here. \n" << std::endl;
}
