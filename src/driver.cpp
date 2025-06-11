#include <iostream> 

#include "parser.h"

int main(){
    Parser parser("testing.txt");
    ASTBase* base = parser.buildSyntaxTree();
    base->dump();
    return 0;
}
