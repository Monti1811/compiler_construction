#pragma once

#include "../util/diagnostic.h"
#include "../util/symbol_internalizer.h"

#define KIND_ACTION(KD, STR) KD,

enum TokenKind {
#include "tokenkinds.def"
};

#undef KIND_ACTION

struct Token : public Locatable {
   public:
    TokenKind Kind;
    Symbol Text;

    Token(Locatable& loc, TokenKind kind, Symbol text);

    friend std::ostream& operator<<(std::ostream& stream, const Token& tok);
};
