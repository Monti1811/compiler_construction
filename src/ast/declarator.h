#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "../util/diagnostic.h"
#include "../util/symbol_internalizer.h"

struct Declarator {
    bool isEmptyDeclarator();
};

struct TypeSpecifier {
    public:
    TypeSpecifier(const Locatable loc) : _loc(loc) {};
    private:
    const Locatable _loc;
};

// int x;
// ^   ^
// |   declarator
// type-specifier
struct Declaration {
    Declaration(std::unique_ptr<TypeSpecifier> spec, std::unique_ptr<Declarator> decl)
        : specifier(std::move(spec))
        , declarator(std::move(decl)) {};

    std::unique_ptr<TypeSpecifier> specifier;
    std::unique_ptr<Declarator> declarator;
};

struct ParameterDeclaration {
    TypeSpecifier specifier;
    std::optional<Declarator> declarator;
};

struct PrimitiveDeclarator: public Declarator {
    PrimitiveDeclarator(Locatable loc, Symbol sym): ident(sym) {};

    Symbol ident;
};

struct FunctionDeclarator : public Declarator {
    FunctionDeclarator(Locatable loc, Declarator* _decl): decl(_decl) {};

    Declarator* decl;
    std::vector<ParameterDeclaration> parameters;

    void addParameter(Declaration param);
};

struct PointerDeclarator : public Declarator {
    PointerDeclarator(Locatable loc, Declarator* _inner): inner(_inner) {};

    // Pointer
    Declarator* inner;
};

struct VoidSpecifier: public TypeSpecifier {
    public: 
    VoidSpecifier(const Locatable loc) : TypeSpecifier(loc) {};
};

struct CharSpecifier: public TypeSpecifier {
    public: 
    CharSpecifier(const Locatable loc) : TypeSpecifier(loc) {};
};

struct IntSpecifier: public TypeSpecifier {
    public: 
    IntSpecifier(const Locatable loc) : TypeSpecifier(loc) {};
};

struct StructSpecifier: public TypeSpecifier {
    public:
    StructSpecifier(const Locatable loc, Symbol tag) : TypeSpecifier(loc), _tag(tag) {};
    void addComponent(Declaration spec_decl);

    private:
    Symbol _tag;
};
