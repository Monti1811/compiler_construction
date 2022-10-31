#include <string>
#include "lexer.h"
#include "../../impl_ideas/diagnostic.h"

Lexer::Lexer(
    std::string filename,
    SymbolInternalizer internalizer,
) : m_internalizer(internalizer) {
    m_stream = LocatableStream(filename);
}

Lexer::Lexer(
    std::string filename,
) {
    m_stream = LocatableStream(filename);
}

Lexer::Lexer(
    SymbolInternalizer internalizer,
) : m_internalizer(internalizer) {}

Token Lexer::next() {
    // TODO
    return readStringLiteral();
}

Token Lexer::readStringLiteral() {
    // Save location
    Locatable loc = m_stream::loc();

    // Read initial quotation mark
    m_stream.get();

    string inner;
    while (char c = m_stream.get() != '"') {
        inner += c;
        if (c == '\\') {
            inner += m_stream.get();
        }
    }

    // TODO: Do we have to disallow newlines inside of strings?
    // e.g.:
    // char* test = "hallo
    // welt";

    Symbol sym = internalizer::internalize(inner);
    return Token(loc, TokenKind::TK_STRING_LITERAL, sym);
}

