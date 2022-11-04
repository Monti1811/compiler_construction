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
    Token readIdentOrKeyword();

    char readEscapeChar();
    Token readCharConstant();
    Token readNumberConstant();

    Token readStringLiteral();

    Token readPunctuator();

    void readMultiComment();
    void readLineComment();

    Token eof();
    template<typename T> Token makeToken(Locatable loc, TokenKind kind, T symbol);

    void fail(std::string message);
    void fail(std::string message, Locatable& loc);
    
    LocatableStream m_stream;
    SymbolInternalizer& m_internalizer;
};
