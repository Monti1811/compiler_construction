#pragma once

#include "declarators/declarator.h"
#include "specifiers/specifier.h"

#include "compile_scope.h"
#include "type_decl.h"

// int x;
// ^   ^
// |   declarator
// type-specifier
struct Declaration {
    Declaration(Locatable loc, TypeSpecifierPtr specifier, DeclaratorPtr declarator)
        : loc(loc)
        , specifier(std::move(specifier))
        , declarator(std::move(declarator)){};

    void print(std::ostream& stream);
    friend std::ostream& operator<<(std::ostream& stream, Declaration& declaration);

    void typecheck(ScopePtr& scope);
    TypeDecl toType(ScopePtr& scope);
    TypeDecl getTypeDecl();

    void compile(CompileScopePtr compile_scope);

    Locatable loc;
    TypeSpecifierPtr specifier;
    DeclaratorPtr declarator;

  private:
    std::optional<TypeDecl> _typeDecl;
};
