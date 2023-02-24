#pragma once

#include "declarator.h"

struct PointerDeclarator : public Declarator {
  public:
    PointerDeclarator(Locatable loc, DeclaratorPtr inner)
        : Declarator(loc, inner->isAbstract(), DeclaratorKind::POINTER)
        , _inner(std::move(inner)){};

    void print(std::ostream& stream);

    std::optional<Symbol> getName();
    TypePtr wrapType(TypePtr const& type, ScopePtr& scope);

    bool isFunction();

  private:
    DeclaratorPtr _inner;
};
