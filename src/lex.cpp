#include <iostream>
#include <string> 
#include "lex.h"

Lexer::Lexer(const char*filename): 
    filename (filename) {
    std::cout << "I am parsing: " << filename << std::endl;
}
