#include <iostream>
#include <string>

#include "ast/parser.h"
#include "lexer/lexer.h"

enum CompilerStage { TOKENIZE, PARSE, COMPILE };

int main(int argc, char const *argv[]) {
    SymbolInternalizer internalizer = SymbolInternalizer();

    bool print_tokens = false;
    bool print_ast = false;

    CompilerStage stage = CompilerStage::COMPILE;

    std::string filename;

    for (auto i = 1; i < argc; i++) {
        auto arg = argv[i];

        auto matches = [&](char const *match) {
            return strcmp(arg, match) == 0;
        };

        if (matches("--tokenize")) {
            print_tokens = true;
            stage = CompilerStage::TOKENIZE;
        } else if (matches("--parse")) {
            stage = CompilerStage::PARSE;
        } else if (matches("--print-ast")) {
            print_ast = true;
            stage = CompilerStage::PARSE;
        } else if (matches("--compile")) {
            stage = CompilerStage::COMPILE;
        } else {
            filename = std::string(arg);
        }
    }

    if (filename.empty()) {
        std::cout << "Syntax: " << argv[0] << " [arguments] <filename>"
                  << std::endl;
        return EXIT_FAILURE;
    }

    // Tokenize input with the lexer
    Lexer lexer(filename, internalizer);

    if (print_tokens) {
        lexer.printTokens();
    }

    if (stage <= CompilerStage::TOKENIZE) {
        return EXIT_SUCCESS;
    }

    // Parse the token stream and typecheck
    Parser parser(lexer);
    auto program = parser.parseProgram();
    program.typecheck();

    if (print_ast) {
        std::cout << program << std::endl;
    }

    if (stage <= CompilerStage::PARSE) {
        return EXIT_SUCCESS;
    }

    // Compile the program into LLVM code
    program.compile(argc, argv, filename);

    return EXIT_SUCCESS;
}
