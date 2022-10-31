#pragma once

#include <fstream>
#include <string>
#include "symbol_internalizer.h"
#include "token.h"
#include "stream.h"

class Lexer {
    public:
    Lexer(std::string filename, SymbolInternalizer internalizer)
        : m_stream(LocatableStream(filename))
        , m_internalizer(internalizer) {}

    Token next();

    private:
    Token readStringLiteral();
    
    LocatableStream m_stream;
    SymbolInternalizer m_internalizer;
};
