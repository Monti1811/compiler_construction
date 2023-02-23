#pragma once

#include "../../util/diagnostic.h"

#include "../scope.h"

enum SpecifierKind { VOID, INT, CHAR, STRUCT };

struct TypeSpecifier {
  public:
    TypeSpecifier(const Locatable loc, const SpecifierKind kind)
        : _loc(loc)
        , _kind(kind){};
    virtual ~TypeSpecifier() = default;

    friend std::ostream& operator<<(std::ostream& stream, const std::unique_ptr<TypeSpecifier>& type);
    virtual void print(std::ostream& stream);

    virtual TypePtr toType(ScopePtr& scope);

    const Locatable _loc;
    SpecifierKind _kind;
};

typedef std::unique_ptr<TypeSpecifier> TypeSpecifierPtr;
