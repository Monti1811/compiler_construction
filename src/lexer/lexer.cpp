#include "lexer.h"

#include <string>

#include "../util/diagnostic.h"

Token Lexer::next() {
    switch (m_stream.peek()) {
        case ' ':
        case '\t':
        case '\n':
        case '\v':
        case '\r': {
            m_stream.get();
            return next();
        }
        case '\'':
            return readCharConstant();
        case '\"':
            return readStringLiteral();
        case EOF:
            return eof();
        default: {
            fail("Unknown token");
            return eof();
        }
    }
}

char Lexer::readEscapeChar() {
    Locatable loc = m_stream.loc();

    switch(char c = m_stream.get()) {
        case '\'':
        case '"':
        case '?':
        case '\\':
        case 'a':
        case 'b':
        case 'f':
        case 'n':
        case 'r':
        case 't':
        case 'v':
            return c;
        case EOF:
            fail("Unexpected end of file");
            return EOF;
        default:
            fail("Invalid escape sequence");
            return EOF;
    }
}

Token Lexer::readCharConstant() {
    Locatable loc = m_stream.loc();

    // Read initial quotation mark
    m_stream.get();

    std::string inner;

    switch (char c = m_stream.get()) {
        case EOF:
            fail("Unexpected end of file");
            break;
        case '\'':
            fail("Character literals must not be empty");
            break;
        case '\n':
            fail("Character literals must not contain a newline");
            break;
        case '\\': {
            inner += c;
            inner += readEscapeChar();
            break;
        }
        default:
            inner += c;
    }

    // Read final quotation mark
    if (m_stream.get() != '\'') {
        fail("Character literals must only contain a single character");
    }

    Symbol sym = m_internalizer.internalize('\'' + inner + '\'');
    return Token(loc, TokenKind::TK_CHARACTER_CONSTANT, sym);
}

Token Lexer::readStringLiteral() {
    // Save location
    Locatable loc = m_stream.loc();

    // Read initial quotation mark
    m_stream.get();

    std::string inner;

    char c = m_stream.get();
    while (c != '"') {
        if (c == EOF) {
            fail("Unexpected end of file");
        }

        inner += c;
        if (c == '\\') {
            inner += readEscapeChar();
        }

        c = m_stream.get();
    }

    // TODO: Do we have to disallow newlines inside of strings?
    // e.g.:
    // char* test = "hallo
    // welt";

    Symbol sym = m_internalizer.internalize('"' + inner + '"');
    return Token(loc, TokenKind::TK_STRING_LITERAL, sym);
}

Token Lexer::eof() {
    Symbol sym = m_internalizer.internalize("");
    Locatable loc = m_stream.loc();
    return Token(loc, TokenKind::TK_EOI, sym);
}

void Lexer::fail(std::string message) {
    errorloc(m_stream.loc(), message);
}
