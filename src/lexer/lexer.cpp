#include "lexer.h"

#include <string>

#include "../util/diagnostic.h"

// Helper functions

bool isNumber(char x);
bool isAlphabetic(char x);
bool isPunctuator(char x);
bool isCommentaryLine(LocatableStream& m_stream);
bool isCommentaryMultiLine(LocatableStream& m_stream);


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
    } else if (isCommentaryLine(m_stream)) {
        std::string str = m_stream.getline();
        std::cout << "Line:" << str << std::endl;
        return next();
    } else if (isCommentaryMultiLine(m_stream)) {
        findEndCommentary();
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
    // char* test = "hallonext_char_val
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
        Symbol sym = m_internalizer.internalize(next_char_val);
        return Token(loc, TokenKind::TK_ZERO_CONSTANT, sym);
    // Character is a digit
    } else {
        // TODO: Should leading zeros be forwarded to the string that is saved?
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
    str += next_char_val;
    char next_char = m_stream.peek();
    // While there is still another valid char add it to the buffer
    while (isNumber(next_char) || isAlphabetic(next_char)) {
        str += next_char;
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

Token Lexer::readPunctuator() {
    char element = m_stream.get();
    Locatable loc = m_stream.loc();
    switch (element) {
        case '*':
        {
            if (m_stream.peek() == '=') {
                m_stream.get();
                Symbol sym = m_internalizer.internalize("*=");
                return Token(loc, TokenKind::TK_ASTERISK_EQUAL, sym);
            } else {
                Symbol sym = m_internalizer.internalize(element);
                return Token(loc, TokenKind::TK_ASTERISK, sym);
            }
        }   
        case ',':
        {
            Symbol sym = m_internalizer.internalize(element);
            return Token(loc, TokenKind::TK_COMMA, sym);
        }
        case ';':
        {
            Symbol sym = m_internalizer.internalize(element);
            return Token(loc, TokenKind::TK_SEMICOLON, sym);
        }
        case '(':
        {
            Symbol sym = m_internalizer.internalize(element);
            return Token(loc, TokenKind::TK_LPAREN, sym);
        }
        case ')':
        {
            Symbol sym = m_internalizer.internalize(element);
            return Token(loc, TokenKind::TK_RPAREN, sym);
        }
        case '{':
        {
            Symbol sym = m_internalizer.internalize(element);
            return Token(loc, TokenKind::TK_LBRACE, sym);
        }
        case '}':
        {
            Symbol sym = m_internalizer.internalize(element);
            return Token(loc, TokenKind::TK_RBRACE, sym);
        }
        case '[':
        {
            Symbol sym = m_internalizer.internalize(element);
            return Token(loc, TokenKind::TK_LBRACKET, sym);
        }
        case ']':
        {
            Symbol sym = m_internalizer.internalize(element);
            return Token(loc, TokenKind::TK_RBRACKET, sym);
        }
        case '?':
        {
            Symbol sym = m_internalizer.internalize(element);
            return Token(loc, TokenKind::TK_QUESTION_MARK, sym);
        }
        case '.':
        {
            // Check if it's '...' or '.'
            if (m_stream.peek() == '.' && m_stream.peek_twice() == '.') {
                m_stream.get();
                m_stream.get();
                Symbol sym = m_internalizer.internalize("...");
                return Token(loc, TokenKind::TK_DOT_DOT_DOT, sym);
            } else {
                Symbol sym = m_internalizer.internalize(element);
                return Token(loc, TokenKind::TK_DOT, sym);
            }
        }
        case '#':
        {
            if (m_stream.peek() == '#') {
                m_stream.get();
                Symbol sym = m_internalizer.internalize("##");
                return Token(loc, TokenKind::TK_POUND, sym);
            } else {
                Symbol sym = m_internalizer.internalize(element);
                return Token(loc, TokenKind::TK_POUND_POUND, sym);
            }
        } 
        // Check if it's '+', '++' or '+='
        case '+':
        {
            switch (m_stream.peek()) {
                case '+': 
                {
                    m_stream.get();
                    Symbol sym = m_internalizer.internalize("++");
                    return Token(loc, TokenKind::TK_PLUSPLUS, sym);
                }
                case '=': 
                {
                    m_stream.get();
                    Symbol sym = m_internalizer.internalize("+=");
                    return Token(loc, TokenKind::TK_PLUS_EQUAL, sym);
                }
                default:
                {
                    Symbol sym = m_internalizer.internalize(element);
                    return Token(loc, TokenKind::TK_PLUS, sym);
                }
            }
        } 
        // Check if it's '-', '--' or '-='
        case '-':
        {
            switch (m_stream.peek()) {
                case '-': 
                {
                    m_stream.get();
                    Symbol sym = m_internalizer.internalize("--");
                    return Token(loc, TokenKind::TK_MINUSMINUS, sym);
                }
                case '=': 
                {
                    m_stream.get();
                    Symbol sym = m_internalizer.internalize("-=");
                    return Token(loc, TokenKind::TK_MINUS_EQUAL, sym);
                }
                default:
                {
                    Symbol sym = m_internalizer.internalize(element);
                    return Token(loc, TokenKind::TK_MINUS, sym);
                }
            }
        } 
        // Check if it's '&', '&&' or '&='
        case '&':
        {
            switch (m_stream.peek()) {
                case '&': 
                {
                    m_stream.get();
                    Symbol sym = m_internalizer.internalize("&&");
                    return Token(loc, TokenKind::TK_AND_AND, sym);
                }
                case '=': 
                {
                    m_stream.get();
                    Symbol sym = m_internalizer.internalize("&=");
                    return Token(loc, TokenKind::TK_AND_EQUAL, sym);
                }
                default:
                {
                    Symbol sym = m_internalizer.internalize(element);
                    return Token(loc, TokenKind::TK_AND, sym);
                }
            }
        } 
        // Check if it's '|', '||' or '|='
        case '|':
        {
            switch (m_stream.peek()) {
                case '|': 
                {
                    m_stream.get();
                    Symbol sym = m_internalizer.internalize("||");
                    return Token(loc, TokenKind::TK_OR_OR, sym);
                }
                case '=': 
                {
                    m_stream.get();
                    Symbol sym = m_internalizer.internalize("|=");
                    return Token(loc, TokenKind::TK_PIPE_EQUAL, sym);
                }
                default:
                {
                    Symbol sym = m_internalizer.internalize(element);
                    return Token(loc, TokenKind::TK_PIPE, sym);
                }
            }
        } 
        case '~':
        {
            Symbol sym = m_internalizer.internalize(element);
            return Token(loc, TokenKind::TK_TILDE, sym);
        }
        case '!':
        {
            if (m_stream.peek() == '=') {
                m_stream.get();
                Symbol sym = m_internalizer.internalize("!=");
                return Token(loc, TokenKind::TK_NOT_EQUAL, sym);
            } else {
                Symbol sym = m_internalizer.internalize(element);
                return Token(loc, TokenKind::TK_BANG, sym);
            }
        }
        case '=':
        {
            if (m_stream.peek() == '=') {
                m_stream.get();
                Symbol sym = m_internalizer.internalize("==");
                return Token(loc, TokenKind::TK_EQUAL_EQUAL, sym);
            } else {
                Symbol sym = m_internalizer.internalize(element);
                return Token(loc, TokenKind::TK_EQUAL, sym);
            }
        }
        case ':':
        {
            if (m_stream.peek() == '>') {
                m_stream.get();
                Symbol sym = m_internalizer.internalize(":>");
                return Token(loc, TokenKind::TK_RBRACKET, sym);
            } else {
                Symbol sym = m_internalizer.internalize(element);
                return Token(loc, TokenKind::TK_COLON, sym);
            }
        }
        case '^':
        {
            if (m_stream.peek() == '=') {
                m_stream.get();
                Symbol sym = m_internalizer.internalize("^=");
                return Token(loc, TokenKind::TK_HAT_EQUAL, sym);
            } else {
                Symbol sym = m_internalizer.internalize(element);
                return Token(loc, TokenKind::TK_HAT, sym);
            }
        }
        case '/':
        {
            if (m_stream.peek() == '=') {
                m_stream.get();
                Symbol sym = m_internalizer.internalize("/=");
                return Token(loc, TokenKind::TK_SLASH_EQUAL, sym);
            } else {
                Symbol sym = m_internalizer.internalize(element);
                return Token(loc, TokenKind::TK_SLASH, sym);
            }
        }
        // Checks if it's a '%', '%=', '%>', '%:' or '%:%:'
        case '%':
        {
            switch (m_stream.peek()) {
                case '=': 
                {
                    m_stream.get();
                    Symbol sym = m_internalizer.internalize("%=");
                    return Token(loc, TokenKind::TK_PERCENT_EQUAL, sym);
                }
                case '>':
                {
                    m_stream.get();
                    Symbol sym = m_internalizer.internalize("%>");
                    return Token(loc, TokenKind::TK_RBRACE, sym);
                }
                case ':':
                {
                    std::string str = m_stream.peek_forward(3);
                    if (str == ":%:") {
                        m_stream.read(3);
                        Symbol sym = m_internalizer.internalize("%:%:");
                        return Token(loc, TokenKind::TK_POUND_POUND, sym);
                    } else {
                        m_stream.get();
                        Symbol sym = m_internalizer.internalize("%:");
                        return Token(loc, TokenKind::TK_POUND, sym);
                    }
                    
                }
                default:
                {
                    Symbol sym = m_internalizer.internalize(element);
                    return Token(loc, TokenKind::TK_PERCENT, sym);
                }
            }
        }
        // Check if it's '<', '<<', '<<=', '<%', '<:' or '<='
        case '<':
        {
            switch (m_stream.peek()) {
                case '<': 
                {
                    if (m_stream.peek_twice() == '=') {
                        m_stream.read(2);
                        Symbol sym = m_internalizer.internalize("<<=");
                        return Token(loc, TokenKind::TK_LESS_LESS_EQUAL, sym);
                    } else {
                        m_stream.get();
                        Symbol sym = m_internalizer.internalize("<<");
                        return Token(loc, TokenKind::TK_LESS_LESS, sym);
                    }
                }
                case '=': 
                {
                    m_stream.get();
                    Symbol sym = m_internalizer.internalize("<=");
                    return Token(loc, TokenKind::TK_LESS_EQUAL, sym);
                }
                case '%': 
                {
                    m_stream.get();
                    Symbol sym = m_internalizer.internalize("<%");
                    return Token(loc, TokenKind::TK_LBRACE, sym);
                }
                case ':': 
                {
                    m_stream.get();
                    Symbol sym = m_internalizer.internalize("<:");
                    return Token(loc, TokenKind::TK_LBRACKET, sym);
                }
                default:
                {
                    Symbol sym = m_internalizer.internalize(element);
                    return Token(loc, TokenKind::TK_LESS, sym);
                }
            }
        } 
        // Check if it's '>', '>>', '>>=' or '>='
        case '>':
        {
            switch (m_stream.peek()) {
                case '<': 
                {
                    if (m_stream.peek_twice() == '=') {
                        m_stream.read(2);
                        Symbol sym = m_internalizer.internalize(">>=");
                        return Token(loc, TokenKind::TK_GREATER_GREATER_EQUAL, sym);
                    } else {
                        m_stream.get();
                        Symbol sym = m_internalizer.internalize(">>");
                        return Token(loc, TokenKind::TK_GREATER_GREATER, sym);
                    }
                }
                case '=': 
                {
                    m_stream.get();
                    Symbol sym = m_internalizer.internalize(">=");
                    return Token(loc, TokenKind::TK_GREATER_EQUAL, sym);
                }
                default:
                {
                    m_stream.get();
                    Symbol sym = m_internalizer.internalize(element);
                    return Token(loc, TokenKind::TK_GREATER, sym);
                }
            }
        } 

        default:
            fail("Invalid punctuator");
            Symbol sym = m_internalizer.internalize("Error");
            return Token(loc, TokenKind::TK_EOI, sym);
    }
}

void Lexer::findEndCommentary() {
    while ((m_stream.peek() != EOF || m_stream.peek() != '*' || m_stream.peek_twice() != '/')) {
        m_stream.get();
    }
    if (m_stream.peek() == EOF) {
        fail("Multiline commentary was not closed!");
    }
    m_stream.read(2);
}


void Lexer::fail(std::string message) {
    errorloc(m_stream.loc(), message);
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

bool isCommentaryLine(LocatableStream& m_stream) {
    return m_stream.peek() == '/' && m_stream.peek_twice() == '/';
}

bool isCommentaryLine(LocatableStream& m_stream) {
    return m_stream.peek() == '/' && m_stream.peek_twice() == '*';
}
