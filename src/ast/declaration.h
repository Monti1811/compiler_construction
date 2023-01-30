#pragma once

#include "declarators/declarator.h"
#include "specifiers/specifier.h"

#include "type_decl.h"

// int x;
// ^   ^
// |   declarator
// type-specifier
struct Declaration {
    Declaration(Locatable loc, TypeSpecifierPtr specifier, DeclaratorPtr declarator)
        : _loc(loc)
        , _specifier(std::move(specifier))
        , _declarator(std::move(declarator)) {};

    void print(std::ostream& stream);
    friend std::ostream& operator<<(std::ostream& stream, Declaration& declaration);

    void typecheck(ScopePtr& scope);
    TypeDecl toType(ScopePtr& scope);
    TypeDecl getTypeDecl();

    Locatable _loc;
    TypeSpecifierPtr _specifier;
    DeclaratorPtr _declarator;
    private:
    std::optional<TypeDecl> _typeDecl;
};
