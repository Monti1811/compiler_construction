#pragma once

#include <fstream>
#include <string>

#include "../util/symbol_internalizer.h"

#include "token.h"
#include "stream.h"

class Lexer {
   public:
    Lexer(std::string const& filename, SymbolInternalizer& internalizer)
        : m_stream(LocatableStream(filename))
        , m_internalizer(internalizer) {}

    Token next();

   private:
    char readEscapeChar();
    Token readCharConstant();
    Token readNumberConstant();
    Token readStringLiteral();
    Token readIdKeyword();
    Token readPunctuator();

    void findCommentEnd(Locatable& loc);
    void findLineCommentEnd(Locatable& loc);

    Token eof();

    void fail(std::string message);
    void fail(std::string message, Locatable& loc);
    
    LocatableStream m_stream;
    SymbolInternalizer& m_internalizer;
};
