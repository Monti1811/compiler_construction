#pragma once

#include "../declaration.h"

#include "specifier.h"

struct StructSpecifier : public TypeSpecifier {
  public:
    StructSpecifier(const Locatable loc, std::optional<Symbol> tag)
        : TypeSpecifier(loc, SpecifierKind::STRUCT)
        , _tag(tag){};

    void print(std::ostream& stream) override;

    TypePtr toType(ScopePtr&);

    void makeComplete();
    void addComponent(Declaration declaration);

  private:
    std::optional<Symbol> _tag;
    std::optional<std::vector<Declaration>> _components;
};
