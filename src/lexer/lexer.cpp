#include "lexer.h"

#include <string>

#include "../util/diagnostic.h"

// Helper functions

bool isNumber(char x);
bool isIdKeyword(char x);



Token Lexer::next() {

    char next_char = m_stream.peek();
    Locatable loc = m_stream.loc();
    if (isNumber(next_char)) {
        return readNumberConstant();
    } else if (isIdKeyword(next_char)) {
        return readIdKeyword();
    }
    switch (next_char) {
        case '*':
            Symbol sym = m_internalizer.internalize("*");
            return Token(loc, TokenKind::TK_ASTERISK, sym);
        case ',':
            Symbol sym = m_internalizer.internalize(",");
            return Token(loc, TokenKind::TK_COMMA, sym);
        case ';':
            Symbol sym = m_internalizer.internalize(";");
            return Token(loc, TokenKind::TK_SEMICOLON, sym);
        case '(':
            Symbol sym = m_internalizer.internalize("(");
            return Token(loc, TokenKind::TK_LPAREN, sym);
        case ')':
            Symbol sym = m_internalizer.internalize(")");
            return Token(loc, TokenKind::TK_RPAREN, sym);
        case '{':
            Symbol sym = m_internalizer.internalize("{");
            return Token(loc, TokenKind::TK_LBRACE, sym);
        case '}':
            Symbol sym = m_internalizer.internalize("}");
            return Token(loc, TokenKind::TK_RBRACE, sym);
        case '\'':
            return readCharConstant();
        case '"':
            return readStringLiteral();
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
        case EOF:
            return eof();
        default: {
            fail("Unknown token");
            return eof();
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


Token Lexer::readNumberConstant() {
    // Save location
    Locatable loc = m_stream.loc();
    char next_char_val = m_stream.get();
    // Check if the character is only a 0 and no other number
    if (next_char_val == '0' && !isNumber(m_stream.peek())) {
        Symbol sym = m_internalizer.internalize("0");
        return Token(loc, TokenKind::TK_ZERO_CONSTANT, sym);
    // Character is a digit
    } else {
        // TODO: Should leading zeros be forwarded to the string that is saved?
        // Buffer for the number
        std::string num;
        // Add the first read character to the buffer
        num.append(1, char(next_char_val));
        char next_char = m_stream.peek();
        // While there is still another number, add it to the buffer and go to the next character
        while (isNumber(next_char)) {
            num.append(1, next_char);
            m_stream.get();
            next_char = m_stream.peek();
        }
        Symbol sym = m_internalizer.internalize(num);
        return Token(loc, TokenKind::TK_DECIMAL_CONSTANT, sym);
    }
}


Token Lexer::readIdKeyword() {
    // Save location
    Locatable loc = m_stream.loc();
    char next_char_val = m_stream.get();
    // Buffer for the number
    std::string str;
    // Add the first read character to the buffer
    str.append(1, next_char_val);
    char next_char = m_stream.peek();
    // While there is still another valid char add it to the buffer
    while (isNumber(next_char) || isIdKeyword(next_char)) {
        str.append(1, next_char);
        m_stream.get();
        next_char = m_stream.peek();
    }
    Symbol sym = m_internalizer.internalize(str);
    TokenKind tok;
    // If the string is a keyword, get the correct token, otherwise it's an identifier
    if (Token::containsKeyword(str)) {
        tok = Token::getKeywordToken(str);
    } else {
        tok = TokenKind::TK_IDENTIFIER;
    }
    return Token(loc, tok, sym);
}



void Lexer::fail(std::string message) {
    errorloc(m_stream.loc(), message);
}


// Helpers

// Checks if wether x is between 0 and 9 in ascii
bool isNumber(char x) {
    if ((int)x > 47 && (int)x < 58) {
        return true;
    }
}

// Checks if a the character is a lower/uppercase letter or underscore
bool isIdKeyword(char x) {
    if ((int)x == 95
        || ((int)x > 96 && (int)x < 123)
        || ((int)x > 64 && (int)x < 91)) {
        return true;
    }
}
