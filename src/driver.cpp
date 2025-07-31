#include "ast.h"
#include "context.h"
#include "parser.h"

Parser parseFile(const char *path_to_file) {
  // FIXME: move this into its own function!
  ContextHolder context = std::make_shared<GlobalContext>(path_to_file);
  Parser parser(context);
  parser.start();

  return parser;
}
