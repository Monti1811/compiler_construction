#include <string>
#include "lexer.h"
#include "diagnostic.h"

Token Lexer::next() {
    // TODO
    return readStringLiteral();
}

Token Lexer::readStringLiteral() {
    // Save location
    Locatable loc = m_stream.loc();

    // Read initial quotation mark
    m_stream.get();

    std::string inner;
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

    Symbol sym = m_internalizer.internalize(inner);
    return Token(loc, TokenKind::TK_STRING_LITERAL, sym);
}

