#include "token.h"
#include <unordered_map>

Token::Token(Locatable& loc, TokenKind kind, Symbol text)
    : Locatable(loc), Kind(kind), Text(text) {}

std::ostream& operator<<(std::ostream& stream, const Token& tok) {
    stream << static_cast<const Locatable&>(tok) << ' ';

#define KIND_ACTION(KD, STR) \
    case KD:                 \
        stream << STR;       \
        break;

    switch (tok.Kind) {
#include "tokenkinds.def"
    }

#undef KIND_ACTION

    stream << ' ' << *(tok.Text);

    return stream;
}

std::unordered_map<std::string, TokenKind> string_to_enum = {
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

bool Token::containsKeyword(std::string str) {
    // Checks the map to see if the token exists
    return string_to_enum.count(str) > 0;
}

TokenKind Token::getKeywordToken(std::string str) {
    return string_to_enum[str];
}