#pragma once

#include "../../util/diagnostic.h"

#include "../scope.h"

enum SpecifierKind { VOID, INT, CHAR, STRUCT };

struct TypeSpecifier {
  public:
    TypeSpecifier(const Locatable loc, const SpecifierKind kind)
        : loc(loc)
        , kind(kind){};
    virtual ~TypeSpecifier() = default;

    friend std::ostream& operator<<(std::ostream& stream, const std::unique_ptr<TypeSpecifier>& type);
    virtual void print(std::ostream& stream);

    virtual TypePtr toType(ScopePtr& scope);

    const Locatable loc;
    SpecifierKind kind;
};

typedef std::unique_ptr<TypeSpecifier> TypeSpecifierPtr;
