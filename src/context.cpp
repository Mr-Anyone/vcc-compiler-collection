#include "context.h"

GlobalContext::GlobalContext()
    : context(), builder(context), module("my module", context),
      symbol_table() {}
