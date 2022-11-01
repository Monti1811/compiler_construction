#include <iostream>

#include "lexer/lexer.h"

using namespace std;

int main(int, char **const) {
    SymbolInternalizer internalizer = SymbolInternalizer();
    std::string filename("test.c");
    Lexer lexer(filename, internalizer);

    Token token = lexer.next();
    while (token.Kind != TokenKind::TK_EOI) {
        cout << token << std::endl;
        token = lexer.next();
    }

    return EXIT_SUCCESS;
}
