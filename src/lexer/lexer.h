#pragma once

#include <fstream>
#include <string>

#include "../util/symbol_internalizer.h"

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
    void fail(std::string message);
    
    LocatableStream m_stream;
    SymbolInternalizer m_internalizer;
};
