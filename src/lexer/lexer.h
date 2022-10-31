#include <fstream>
#include <string>
#include "../../impl_ideas/symbol_internalizer.h"
#include "token.h"

struct Lexer {
    public:
    Lexer(std::string&, SymbolInternalizer&);
    Lexer(std::string);
    Lexer(SymbolInternalizer);

    Token next();

    private:
    std::ifstream m_stream;
    SymbolInternalizer m_internalizer;
};
