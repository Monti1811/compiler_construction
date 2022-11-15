#pragma once

#include <string>
#include <vector>

#include "../util/diagnostic.h"

struct Declarator {
    bool isEmptyDeclarator();
};

struct Specifier {
    public:
    Specifier(const Locatable loc) : _loc(loc) {};
    private:
    const Locatable _loc;
};

struct SpecDecl {
    SpecDecl(Specifier* spec, Declarator* decl) : specifier(spec), declarator(decl) {};

    Specifier* specifier;
    Declarator* declarator;
};

struct ParameterList {};

struct BaseParameter : public ParameterList {
    Declarator inner;
};

struct MultipleParameter : public ParameterList {
    BaseParameter inner;
    ParameterList nextParameter;
};

struct PrimitiveDeclarator : public Declarator {
    PrimitiveDeclarator(Locatable loc, Symbol _ident): ident(_ident) {};

    // Identifier
    Symbol ident;
};

struct AbstractDeclarator : public Declarator {};

struct PointerDeclarator : public AbstractDeclarator {
    PointerDeclarator(Locatable loc, Declarator* _inner): inner(_inner) {};

    // Pointer
    Declarator* inner;
};

struct DirectAbstractDeclarator : public AbstractDeclarator {};

struct DirectAbstractDeclaratorStar : public DirectAbstractDeclarator {
    DirectAbstractDeclarator inner;
    ParameterList parameters;
};

struct DirectAbstractDeclaratorParameter : public DirectAbstractDeclarator {
    DirectAbstractDeclarator inner;
};

struct FunctionDeclarator : public Declarator {
    FunctionDeclarator(Locatable loc, Declarator* _decl): decl(_decl) {};

    Declarator* decl;
    ParameterList parameters;

    void addParameter(SpecDecl* param);
};

struct VoidSpecifier: public Specifier {
    public: 
    VoidSpecifier(const Locatable loc) : Specifier(loc) {};
};

struct CharSpecifier: public Specifier {
    public: 
    CharSpecifier(const Locatable loc) : Specifier(loc) {};
};

struct IntSpecifier: public Specifier {
    public: 
    IntSpecifier(const Locatable loc) : Specifier(loc) {};
};

struct StructSpecifier: public Specifier {
    public:
    StructSpecifier(const Locatable loc, Symbol tag) : Specifier(loc), _tag(tag) {};
    void addComponent(SpecDecl*& spec_decl);

    private:
    Symbol _tag;
};
