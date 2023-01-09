#pragma once

#include "declarator.h"

#include "../declaration.h"
#include "../types.h"

struct FunctionDeclarator : public Declarator {
    public:
    FunctionDeclarator(Locatable loc, DeclaratorPtr decl)
        : Declarator(loc, decl->isAbstract(), DeclaratorKind::FUNCTION)
        , _name(std::move(decl)) {};

    void print(std::ostream& stream);

    std::optional<Symbol> getName();
    TypePtr wrapType(TypePtr const& type, ScopePtr& scope);

    bool isFunction();

    void addParameter(Declaration param);

    DeclaratorPtr _name;
    std::vector<Declaration> _parameters;
};
