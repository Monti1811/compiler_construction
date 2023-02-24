#include "token.h"
#include <unordered_map>

Token::Token(Locatable& loc, TokenKind kind, Symbol text)
    : Locatable(loc)
    , kind(kind)
    , text(text) {}

std::ostream& operator<<(std::ostream& stream, const Token& tok) {
    stream << static_cast<const Locatable&>(tok) << ' ';

#define KIND_ACTION(KD, STR)                                                                                           \
    case KD:                                                                                                           \
        stream << STR;                                                                                                 \
        break;

    switch (tok.kind) {
#include "tokenkinds.def"
    }

#undef KIND_ACTION

    stream << ' ' << *(tok.text);

    return stream;
}

const std::unordered_map<std::string, TokenKind> KEYWORDS = {
    {"auto", TK_AUTO},
    {"break", TK_BREAK},
    {"case", TK_CASE},
    {"char", TK_CHAR},
    {"const", TK_CONST},
    {"continue", TK_CONTINUE},
    {"default", TK_DEFAULT},
    {"do", TK_DO},
    {"double", TK_DOUBLE},
    {"else", TK_ELSE},
    {"enum", TK_ENUM},
    {"extern", TK_EXTERN},
    {"float", TK_FLOAT},
    {"for", TK_FOR},
    {"goto", TK_GOTO},
    {"if", TK_IF},
    {"inline", TK_INLINE},
    {"int", TK_INT},
    {"long", TK_LONG},
    {"register", TK_REGISTER},
    {"restrict", TK_RESTRICT},
    {"return", TK_RETURN},
    {"short", TK_SHORT},
    {"signed", TK_SIGNED},
    {"sizeof", TK_SIZEOF},
    {"static", TK_STATIC},
    {"struct", TK_STRUCT},
    {"switch", TK_SWITCH},
    {"typedef", TK_TYPEDEF},
    {"union", TK_UNION},
    {"unsigned", TK_UNSIGNED},
    {"void", TK_VOID},
    {"volatile", TK_VOLATILE},
    {"while", TK_WHILE},
    {"_Alignas", TK__ALIGNAS},
    {"_Alignof", TK__ALIGNOF},
    {"_Atomic", TK__ATOMIC},
    {"_Bool", TK__BOOL},
    {"_Complex", TK__COMPLEX},
    {"_Generic", TK__GENERIC},
    {"_Imaginary", TK__IMAGINARY},
    {"_Noreturn", TK__NORETURN},
    {"_Static_assert", TK__STATIC_ASSERT},
    {"_Thread_local", TK__THREAD_LOCAL},
};

bool Token::isKeyword(std::string str) {
    // Checks the map to see if the token exists
    return KEYWORDS.count(str) > 0;
}

TokenKind Token::getKeywordToken(std::string str) {
    return KEYWORDS.at(str);
}

const std::unordered_map<char, TokenKind> PUNCTUATORS = {
    {'*', TK_ASTERISK}, {',', TK_COMMA},  {';', TK_SEMICOLON}, {'(', TK_LPAREN},   {')', TK_RPAREN},
    {'{', TK_LBRACE},   {'}', TK_RBRACE}, {'[', TK_LBRACKET},  {']', TK_RBRACKET}, {'?', TK_QUESTION_MARK},
    {'.', TK_DOT},      {'#', TK_POUND},  {'+', TK_PLUS},      {'-', TK_MINUS},    {'&', TK_AND},
    {'|', TK_PIPE},     {'~', TK_TILDE},  {'!', TK_BANG},      {'=', TK_EQUAL},    {':', TK_COLON},
    {'^', TK_HAT},      {'/', TK_SLASH},  {'%', TK_PERCENT},   {'<', TK_LESS},     {'>', TK_GREATER},
};

bool Token::isPunctuator(char ch) {
    return PUNCTUATORS.count(ch) > 0;
}

TokenKind Token::getPunctuatorToken(char ch) {
    return PUNCTUATORS.at(ch);
}
