#include "context.h"
#include "ast.h"
#include "lex.h"
#include "stream.h"

using namespace vcc;

GlobalContext::GlobalContext(const char *path_to_file)
    : context(), builder(context), module("my module", context), symbol_table(),
      diagnostics(), stream(path_to_file) {}

void DiagnosticDriver::diag(const std::string &message) {
  setError();

  std::cerr << message << std::endl;
}

void DiagnosticDriver::diag(lex::Tokenizer &tokenizer,
                            const std::string &message) {
  setError();
  printFilePos(tokenizer.getPos(), message);

  std::string line = tokenizer.getLine(tokenizer.getPos());
  std::cerr << line << "\n";
  printSeeHere(tokenizer.getPos());
}

void DiagnosticDriver::diag(const ASTBase *node,
                            const std::string &line_in_file,
                            const std::string &message) {
  setError();
  printFilePos(node->getPos(), message);
  std::cerr << line_in_file << "\n";
  printSeeHere(node->getPos());
}

void DiagnosticDriver::setError() { m_error = true; }

bool DiagnosticDriver::hasError() const { return m_error; }

void DiagnosticDriver::printFilePos(const FilePos &pos,
                                    const std::string &message) {
  std::cerr << pos.row << ":" << pos.col << " Error: " << message << "\n";
}

void DiagnosticDriver::printSeeHere(const FilePos &pos) {
  for (int i = 0; i < pos.col - 1; ++i) {
    std::cerr << " ";
  }

  std::cerr << "^---see here. \n" << std::endl;
}
