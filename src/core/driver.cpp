#include "core/driver.h"
#include "core/ast.h"
#include "core/context.h"
#include "core/parser.h"

vcc::Parser vcc::parseFile(const char *path_to_file) {
  // FIXME: move this into its own function!
  ContextHolder context = std::make_shared<GlobalContext>(path_to_file);
  Parser parser(context);
  parser.start();

  return parser;
}
