#include <iostream>

#include "lexer.h"

using namespace std;

int main(int, char **const) {
    cerr << "TODO: Implement a compiler." << std::endl;

    SymbolInternalizer internalizer = SymbolInternalizer();
    std::string filename("test.c");
    Lexer lexer(filename, internalizer);

    Token test = lexer.next();
    cout << test << std::endl;

    return EXIT_FAILURE;
}
