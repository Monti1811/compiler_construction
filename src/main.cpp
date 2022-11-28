#include <iostream>

#include "lexer/lexer.h"
#include "ast/parser.h"
#include <string.h>

using namespace std;


int main(int argc, char const* argv[]) {
    SymbolInternalizer internalizer = SymbolInternalizer();
    if (argc < 2) {
        cout << "You need to add at least a filename!" << std::endl;
        return EXIT_FAILURE;
    }
    std::string filename(argv[argc-1]);
    // cout << "Filename: " << filename << std::endl;
    Lexer lexer(filename, internalizer);

    for (int i = 1; i < argc - 1; i++) {
        if (strcmp(argv[i], "--tokenize") == 0) {
            // cout << "Start to tokenize the chosen file" << std::endl;
            Token token = lexer.next();
            while (token.Kind != TokenKind::TK_EOI) {
                cout << token << std::endl;
                token = lexer.next();
            }
        } else if (strcmp(argv[i], "--parse") == 0) {
            Parser parser(lexer);
            auto program = parser.parseProgram();
        } else if (strcmp(argv[i], "--print-ast") == 0) {
            Parser parser(lexer);
            auto program = parser.parseProgram();
            std::cout << program << std::endl;
        }    
    }
    

    return EXIT_SUCCESS;
}
