#include "lexer.h"

#include <string>
#include <unordered_map>

#include "../util/diagnostic.h"

// Helper functions

bool isNumber(char x);
bool isAlphabetic(char x);
bool isPunctuator(char x);
bool isCommentLine(LocatableStream& m_stream);
bool isCommentMultiLine(LocatableStream& m_stream);


//TODO omitted tokens should probably not stop the entire lexing process
Token Lexer::next() {

    char next_char = m_stream.peek();

    Locatable loc = m_stream.loc();

    switch (next_char) {
        case ' ':
        case '\t':
        case '\n':
        case '\v':
        {
            m_stream.get();
            return next();
        }
        case '\r': 
        {
            m_stream.get();
            if (m_stream.peek() == '\n') {
                m_stream.get();
            }
            return next();
        }       
        case '\'':
            return readCharConstant();
        case '\"':
            return readStringLiteral();
        case EOF:
            return eof();
        default: {
        }
    }

    if (isNumber(next_char)) {
        return readNumberConstant();
    } else if (isAlphabetic(next_char)) {
        return readIdKeyword();
    } else if (isCommentLine(m_stream)) {
        m_stream.read(2);
        findEndLineComment(loc);
        return next();
    } else if (isCommentMultiLine(m_stream)) {
        m_stream.read(2);
        findEndComment();
        return next();
    } else if (isPunctuator(next_char)) {
        return readPunctuator();
    }
    fail("Unknown token");
    return eof();
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

    // TODO: Is it correct to save the character constants as a string? Especially as the 
    // special symbols are split so they won't work the same way as they would otherwise
    // i.e: "\" + "n" != "\n"
    std::string inner;

    switch (char c = m_stream.get()) {
        case EOF:
            fail("Unexpected end of file", loc);
            break;
        case '\'':
            fail("Character literals must not be empty", loc);
            break;
        case '\n':
            fail("Character literals must not contain a newline", loc);
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
        fail("Character literals must only contain a single character", loc);
    }

    return makeToken(loc, TokenKind::TK_CHARACTER_CONSTANT, '\'' + inner + '\'');
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
            fail("Unexpected end of file", loc);
        }

        inner += c;
        if (c == '\\') {
            inner += readEscapeChar();
        }

        c = m_stream.get();
    }

    // TODO: Do we have to disallow newlines inside of strings?
    // e.g.:
    // char* test = "hallonext_char_val
    // welt";

    return makeToken(loc, TokenKind::TK_STRING_LITERAL, '"' + inner + '"');
}

Token Lexer::eof() {
    Locatable loc = m_stream.loc();
    return makeToken(loc, TokenKind::TK_EOI, "");
}

template<typename T>
Token Lexer::makeToken(Locatable loc, TokenKind kind, T symbol) {
    Symbol sym = m_internalizer.internalize(symbol);
    return Token(loc, kind, sym);
}

Token Lexer::readNumberConstant() {
    // Save location
    Locatable loc = m_stream.loc();
    char next_char_val = m_stream.get();
    // Check if the character is a 0 
    if (next_char_val == '0') {
        return makeToken(loc, TokenKind::TK_ZERO_CONSTANT, next_char_val);
    // Character is a digit
    } else {
        // Buffer for the number
        std::string num;
        // Add the first read character to the buffer
        num += next_char_val;
        char next_char = m_stream.peek();
        // While there is still another number, add it to the buffer and go to the next character
        while (isNumber(next_char)) {
            num += next_char;
            m_stream.get();
            next_char = m_stream.peek();
        }

        return makeToken(loc, TokenKind::TK_DECIMAL_CONSTANT, num);
    }
}


Token Lexer::readIdKeyword() {
    // Save location
    Locatable loc = m_stream.loc();
    char next_char_val = m_stream.get();
    // Buffer for the number
    std::string str;
    // Add the first read character to the buffer
    str += next_char_val;
    char next_char = m_stream.peek();
    // While there is still another valid char add it to the buffer
    while (isNumber(next_char) || isAlphabetic(next_char)) {
        str += next_char;
        m_stream.get();
        next_char = m_stream.peek();
    }

    TokenKind tok;
    // If the string is a keyword, get the correct token, otherwise it's an identifier
    if (Token::containsKeyword(str)) {
        tok = Token::getKeywordToken(str);
    } else {
        tok = TokenKind::TK_IDENTIFIER;
    }

    return makeToken(loc, tok, str);
}

const std::unordered_map<char, TokenKind> PUNCTUATOR_TOKENS = {
    {'*', TK_ASTERISK},
    {',', TK_COMMA},
    {';', TK_SEMICOLON},
    {'(', TK_LPAREN},
    {')', TK_RPAREN},
    {'{', TK_LBRACE},
    {'}', TK_RBRACE},
    {'[', TK_LBRACKET},
    {']', TK_RBRACKET},
    {'?', TK_QUESTION_MARK},
    {'.', TK_DOT},
    {'#', TK_POUND},
    {'+', TK_PLUS},
    {'-', TK_MINUS},
    {'&', TK_AND},
    {'|', TK_PIPE},
    {'~', TK_TILDE},
    {'!', TK_BANG},
    {'=', TK_EQUAL},
    {':', TK_COLON},
    {'^', TK_HAT},
    {'/', TK_SLASH},
    {'%', TK_PERCENT},
    {'<', TK_LESS},
    {'>', TK_GREATER},
};

Token Lexer::readPunctuator() {
    char element = m_stream.get();
    Locatable loc = m_stream.loc();

    if (PUNCTUATOR_TOKENS.count(element) == 0) {
        fail("Invalid punctuator"); // TODO: fail with loc
        return eof();
    }

    TokenKind kind = PUNCTUATOR_TOKENS.at(element);
    std::string symbol(1, element);

    auto setToken = [&](TokenKind _kind, size_t length = 1) {
        kind = _kind;
        symbol += m_stream.read(length);
    };

    switch (kind) {
        case TK_ASTERISK: {
            if (m_stream.peek() == '=') {
                setToken(TokenKind::TK_ASTERISK_EQUAL);
            }
            break;
        }
        case TK_DOT: {
            if (m_stream.peek() == '.' && m_stream.peek_twice() == '.') {
                setToken(TokenKind::TK_DOT_DOT_DOT, 2);
            }
            break;
        }
        case TK_POUND: {
            if (m_stream.peek() == '#') {
                setToken(TokenKind::TK_POUND_POUND);
            }
            break;
        } 
        // Check if it's '+', '++' or '+='
        case TK_PLUS: {
            switch (m_stream.peek()) {
                case '+':
                    setToken(TokenKind::TK_PLUSPLUS);
                    break;
                case '=':
                    setToken(TokenKind::TK_PLUS_EQUAL);
                    break;
            }
            break;
        } 
        // Check if it's '-', '--' or '-='
        case TK_MINUS: {
            switch (m_stream.peek()) {
                case '-':
                    setToken(TokenKind::TK_MINUSMINUS);
                    break;
                case '=':
                    setToken(TokenKind::TK_MINUS_EQUAL);
                    break;
            }
            break;
        } 
        // Check if it's '&', '&&' or '&='
        case TK_AND: {
            switch (m_stream.peek()) {
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
            switch (m_stream.peek()) {
                case '|':
                    setToken(TokenKind::TK_OR_OR);
                    break;
                case '=':
                    setToken(TokenKind::TK_PIPE_EQUAL);
                    break;
            }
            break;
        } 
        case TK_BANG: {
            if (m_stream.peek() == '=') {
                setToken(TokenKind::TK_NOT_EQUAL);
            }
            break;
        }
        case TK_EQUAL: {
            if (m_stream.peek() == '=') {
                setToken(TokenKind::TK_NOT_EQUAL);
            }
            break;
        }
        case TK_COLON: {
            if (m_stream.peek() == '>') {
                setToken(TokenKind::TK_RBRACKET);
            }
            break;
        }
        case TK_HAT: {
            if (m_stream.peek() == '=') {
                setToken(TokenKind::TK_HAT_EQUAL);
            }
            break;
        }
        case TK_SLASH: {
            if (m_stream.peek() == '=') {
                setToken(TokenKind::TK_SLASH_EQUAL);
            }
            break;
        }
        // Checks if it's a '%', '%=', '%>', '%:' or '%:%:'
        case TK_PERCENT: {
            switch (m_stream.peek()) {
                case '=':
                    setToken(TokenKind::TK_PERCENT_EQUAL);
                    break;
                case '>':
                    setToken(TokenKind::TK_RBRACE);
                    break;
                case ':': {
                    std::string str = m_stream.peek_forward(3);
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
            switch (m_stream.peek()) {
                case '<': {
                    if (m_stream.peek_twice() == '=') {
                        setToken(TokenKind::TK_LESS_LESS_EQUAL, 2);
                    } else {
                        setToken(TK_LESS_LESS);
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
        case TK_GREATER:
        {
            switch (m_stream.peek()) {
                case '<': {
                    if (m_stream.peek_twice() == '=') {
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

void Lexer::findCommentEnd(Locatable& loc) {
    if (m_stream.peek() == EOF) {
        fail("Multiline commentary was not closed!", loc);
    }
    if (m_stream.peek_forward(2) == "*/") {
        m_stream.read(2);
        return;
    }
    m_stream.get();
    findCommentEnd(loc);
}

bool isEndOfLine(LocatableStream& m_stream, Locatable& loc) {
    switch (m_stream.peek()) {
        case '\n':
            return true;
        case '\r':
        {
            if (m_stream.peek() == '\n') {
                m_stream.get();
                return true;
            } else {
                return true;
            }
        }
        case EOF:
        {
            errorloc(loc, "Line commentary was not closed!");
            return false;
        }
    }
    return false;
}

void Lexer::findLineCommentEnd(Locatable& loc) {
    char next_char = m_stream.peek();
    bool multiline_comment = false;
    if (next_char == '\\') {
        m_stream.get();
        multiline_comment = true;
    }
    m_stream.get();
    if (isEndOfLine(m_stream, loc) && !multiline_comment) {
        return;
    }
    findLineCommentEnd(loc);
}


void Lexer::fail(std::string message) {
    errorloc(m_stream.loc(), message);
}

void Lexer::fail(std::string message, Locatable& loc) {
    errorloc(loc, message);
}

// Helpers

// Checks if wether x is between 0 and 9 in ascii
bool isNumber(char x) {
    if  (x >= '0' && x <= '9') {
        return true;
    }
    return false;
}

// Checks if a the character is a lower/uppercase letter or underscore
bool isAlphabetic(char x) {
    if (x == '_'
        || (x >= 'a'  && x <= 'z')
        || (x >= 'A' && x <= 'Z')) {
        return true;
    }
    return false;
}

// Checks if a the character is a punctuator
bool isPunctuator(char x) {
    if ((x >= '!' && x <= '/')
        || (x >= ':' && x <= '?')
        || (x >= '[' && x <= '^')
        || (x >= '{' && x <= '~')) {
        return true;
    }
    return false;
}

bool isCommentLine(LocatableStream& m_stream) {
    return m_stream.peek() == '/' && m_stream.peek_twice() == '/';
}

bool isCommentMultiLine(LocatableStream& m_stream) {
    return m_stream.peek() == '/' && m_stream.peek_twice() == '*';
}
