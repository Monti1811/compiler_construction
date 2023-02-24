#include "lexer.h"

#include <string>

#include "../util/diagnostic.h"

// Helper functions

bool isNumber(unsigned char x);
bool isAlphabetic(unsigned char x);

void Lexer::printTokens(void) {
    Token token = this->next();
    while (token.kind != TokenKind::TK_EOI) {
        std::cout << token << std::endl;
        token = this->next();
    }
}

Token Lexer::next() {
    while (true) {
        unsigned char next_char = this->_stream.peek();

        switch (next_char) {
            case ' ':
            case '\t':
            case '\n':
            case '\r':
            case '\v': {
                this->_stream.get();
                continue;
            }
            case '\'':
                return readCharConstant();
            case '\"':
                return readStringLiteral();
            case '\0':
                return eof();
        }

        // [0-9]
        if (isNumber(next_char)) {
            return readNumberConstant();
        }

        // [a-zA-Z_]
        if (isAlphabetic(next_char)) {
            return readIdentOrKeyword();
        }

        // Comments
        if (next_char == '/') {
            unsigned char comment_char = this->_stream.peek(1);
            if (comment_char == '/') {
                readLineComment();
                continue;
            } else if (comment_char == '*') {
                readMultiComment();
                continue;
            }
        }

        // Special characters
        if (Token::isPunctuator(next_char)) {
            return readPunctuator();
        }

        fail("Unknown token");
        return eof();
    }
}

Token Lexer::readIdentOrKeyword() {
    // Save location
    Locatable loc = this->_stream.loc();

    // Buffer for the number
    std::string str;

    // Add the first read character to the buffer
    unsigned char next_char = this->_stream.get();
    str += next_char;

    // While there is still another valid char, add it to the buffer
    next_char = this->_stream.peek();
    while (isNumber(next_char) || isAlphabetic(next_char)) {
        str += next_char;
        this->_stream.get();
        next_char = this->_stream.peek();
    }

    TokenKind kind;
    // If the string is a keyword, get the correct token, otherwise it's an identifier
    if (Token::isKeyword(str)) {
        kind = Token::getKeywordToken(str);
    } else {
        kind = TokenKind::TK_IDENTIFIER;
    }

    return makeToken(loc, kind, str);
}

unsigned char Lexer::readEscapeChar() {
    Locatable loc = this->_stream.loc();

    switch (unsigned char c = this->_stream.get()) {
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
        case '\0':
            fail("Unexpected end of file");
            return '\0';
        default:
            fail("Invalid escape sequence", loc);
            return '\0';
    }
}

Token Lexer::readCharConstant() {
    Locatable loc = this->_stream.loc();

    // Read initial quotation mark
    this->_stream.get();

    Locatable err_loc = this->_stream.loc();

    std::string inner;

    switch (unsigned char c = this->_stream.get()) {
        case '\0':
            fail("Unexpected end of file", err_loc);
            break;
        case '\'':
            fail("Character literals must not be empty", err_loc);
            break;
        case '\n':
            fail("Character literals must not contain a newline", err_loc);
            break;
        case '\\': {
            inner += c;
            inner += readEscapeChar();
            break;
        }
        default:
            inner += c;
    }

    err_loc = this->_stream.loc();

    // Read final quotation mark
    if (this->_stream.get() != '\'') {
        fail("Character literals must only contain a single character", err_loc);
    }

    return makeToken(loc, TokenKind::TK_CHARACTER_CONSTANT, '\'' + inner + '\'');
}

Token Lexer::readNumberConstant() {
    // Save location
    Locatable loc = this->_stream.loc();
    unsigned char next_char_val = this->_stream.get();
    // Check if the character is a 0
    if (next_char_val == '0') {
        return makeToken(loc, TokenKind::TK_ZERO_CONSTANT, next_char_val);
        // Character is a digit
    } else {
        // Buffer for the number
        std::string num;
        // Add the first read character to the buffer
        num += next_char_val;
        unsigned char next_char = this->_stream.peek();
        // While there is still another number, add it to the buffer and go to the next character
        while (isNumber(next_char)) {
            num += next_char;
            this->_stream.get();
            next_char = this->_stream.peek();
        }

        return makeToken(loc, TokenKind::TK_DECIMAL_CONSTANT, num);
    }
}

Token Lexer::readStringLiteral() {
    // Save location
    Locatable loc = this->_stream.loc();

    // Read initial quotation mark
    this->_stream.get();

    std::string inner;

    unsigned char c = this->_stream.peek();
    while (c != '"') {
        if (c == '\0') {
            fail("Unexpected end of file");
        } else if (c == '\n') {
            fail("String literals must not contain newline characters");
        }

        this->_stream.get();
        inner += c;
        if (c == '\\') {
            inner += readEscapeChar();
        }

        c = this->_stream.peek();
    }

    this->_stream.get();

    return makeToken(loc, TokenKind::TK_STRING_LITERAL, '"' + inner + '"');
}

Token Lexer::readPunctuator() {
    Locatable loc = this->_stream.loc();

    char ch = this->_stream.get();
    TokenKind kind = Token::getPunctuatorToken(ch);
    std::string symbol(1, ch);

    auto setToken = [&](TokenKind new_kind, size_t length = 1) {
        kind = new_kind;
        symbol += this->_stream.getStr(length);
    };

    switch (kind) {
        case TK_ASTERISK: {
            if (this->_stream.peek() == '=') {
                setToken(TokenKind::TK_ASTERISK_EQUAL);
            }
            break;
        }
        case TK_DOT: {
            if (this->_stream.peekStr(2) == "..") {
                setToken(TokenKind::TK_DOT_DOT_DOT, 2);
            }
            break;
        }
        case TK_POUND: {
            if (this->_stream.peek() == '#') {
                setToken(TokenKind::TK_POUND_POUND);
            }
            break;
        }
        // Check if it's '+', '++' or '+='
        case TK_PLUS: {
            switch (this->_stream.peek()) {
                case '+':
                    setToken(TokenKind::TK_PLUS_PLUS);
                    break;
                case '=':
                    setToken(TokenKind::TK_PLUS_EQUAL);
                    break;
            }
            break;
        }
        // Check if it's '-', '->', '--' or '-='
        case TK_MINUS: {
            switch (this->_stream.peek()) {
                case '>':
                    setToken(TokenKind::TK_ARROW);
                    break;
                case '-':
                    setToken(TokenKind::TK_MINUS_MINUS);
                    break;
                case '=':
                    setToken(TokenKind::TK_MINUS_EQUAL);
                    break;
            }
            break;
        }
        // Check if it's '&', '&&' or '&='
        case TK_AND: {
            switch (this->_stream.peek()) {
                case '&':
                    setToken(TokenKind::TK_AND_AND);
                    break;
                case '=':
                    setToken(TokenKind::TK_AND_EQUAL);
                    break;
            }
            break;
        }
        // Check if it's '|', '||' or '|='
        case TK_PIPE: {
            switch (this->_stream.peek()) {
                case '|':
                    setToken(TokenKind::TK_PIPE_PIPE);
                    break;
                case '=':
                    setToken(TokenKind::TK_PIPE_EQUAL);
                    break;
            }
            break;
        }
        case TK_BANG: {
            if (this->_stream.peek() == '=') {
                setToken(TokenKind::TK_NOT_EQUAL);
            }
            break;
        }
        case TK_EQUAL: {
            if (this->_stream.peek() == '=') {
                setToken(TokenKind::TK_EQUAL_EQUAL);
            }
            break;
        }
        case TK_COLON: {
            if (this->_stream.peek() == '>') {
                setToken(TokenKind::TK_RBRACKET);
            }
            break;
        }
        case TK_HAT: {
            if (this->_stream.peek() == '=') {
                setToken(TokenKind::TK_HAT_EQUAL);
            }
            break;
        }
        case TK_SLASH: {
            if (this->_stream.peek() == '=') {
                setToken(TokenKind::TK_SLASH_EQUAL);
            }
            break;
        }
        // Checks if it's a '%', '%=', '%>', '%:' or '%:%:'
        case TK_PERCENT: {
            switch (this->_stream.peek()) {
                case '=':
                    setToken(TokenKind::TK_PERCENT_EQUAL);
                    break;
                case '>':
                    setToken(TokenKind::TK_RBRACE);
                    break;
                case ':': {
                    std::string str = this->_stream.peekStr(3);
                    if (str == ":%:") {
                        setToken(TokenKind::TK_POUND_POUND, 3);
                    } else {
                        setToken(TokenKind::TK_POUND);
                    }
                    break;
                }
            }
            break;
        }
        // Check if it's '<', '<<', '<<=', '<%', '<:' or '<='
        case TK_LESS: {
            switch (this->_stream.peek()) {
                case '<': {
                    if (this->_stream.peek(1) == '=') {
                        setToken(TokenKind::TK_LESS_LESS_EQUAL, 2);
                    } else {
                        setToken(TokenKind::TK_LESS_LESS);
                    }
                    break;
                }
                case '=':
                    setToken(TokenKind::TK_LESS_EQUAL);
                    break;
                case '%':
                    setToken(TokenKind::TK_LBRACE);
                    break;
                case ':':
                    setToken(TokenKind::TK_LBRACKET);
                    break;
            }
            break;
        }
        // Check if it's '>', '>>', '>>=' or '>='
        case TK_GREATER: {
            switch (this->_stream.peek()) {
                case '>': {
                    if (this->_stream.peek(1) == '=') {
                        setToken(TokenKind::TK_GREATER_GREATER_EQUAL, 2);
                    } else {
                        setToken(TokenKind::TK_GREATER_GREATER);
                    }
                    break;
                }
                case '=':
                    setToken(TokenKind::TK_GREATER_EQUAL);
                    break;
                default:
                    break;
            }
        }
        default:
            break;
    }

    return makeToken(loc, kind, symbol);
}

void Lexer::readMultiComment() {
    Locatable loc = this->_stream.loc();

    // read initial `/*`
    this->_stream.getStr(2);

    while (true) {
        unsigned char c = this->_stream.peek();

        if (c == '\0') {
            fail("Multiline comment was not closed");
        }

        this->_stream.get();
        if (c == '*') {
            while (true) {
                c = this->_stream.get();
                if (c == '/') {
                    return;
                } else if (c != '*') {
                    break;
                }
            }
        }
    }
}

void Lexer::readLineComment() {
    // The '//' has already been peeked - we can just read everything until the next newline
    this->_stream.getLine();
}

Token Lexer::eof() {
    Locatable loc = this->_stream.loc();
    return makeToken(loc, TokenKind::TK_EOI, "");
}

template <typename T> Token Lexer::makeToken(Locatable loc, TokenKind kind, T symbol) {
    Symbol sym = this->_internalizer.internalize(symbol);
    return Token(loc, kind, sym);
}

void Lexer::fail(std::string message) {
    errorloc(this->_stream.loc(), message);
}

void Lexer::fail(std::string message, Locatable& loc) {
    errorloc(loc, message);
}

// Helpers

// Checks if wether x is between 0 and 9 in ascii
bool isNumber(unsigned char x) {
    return (x >= '0' && x <= '9');
}

// Checks if a the character is a lower/uppercase letter or underscore
bool isAlphabetic(unsigned char x) {
    return (x == '_') || (x >= 'a' && x <= 'z') || (x >= 'A' && x <= 'Z');
}
