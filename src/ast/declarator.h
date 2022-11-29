#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "../util/diagnostic.h"
#include "../util/symbol_internalizer.h"

struct Declarator {
    public:
    Declarator(const Locatable loc, const bool abstract)
        : _loc(loc)
        , _abstract(abstract) {};

    virtual void print(std::ostream& stream) = 0;
    friend std::ostream& operator<<(std::ostream& stream, const std::unique_ptr<Declarator>& declarator);

    bool isAbstract();

    private:
    const Locatable _loc;
    bool _abstract = false;
};

typedef std::unique_ptr<Declarator> DeclaratorPtr;

struct TypeSpecifier {
    public:
    TypeSpecifier(const Locatable loc) : _loc(loc) {};
    virtual ~TypeSpecifier() = default;

    virtual void print(std::ostream& stream) = 0;
    friend std::ostream& operator<<(std::ostream& stream, const std::unique_ptr<TypeSpecifier>& type);

    private:
    const Locatable _loc;
};

typedef std::unique_ptr<TypeSpecifier> TypeSpecifierPtr;

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

    Locatable _loc;
    TypeSpecifierPtr _specifier;
    DeclaratorPtr _declarator;
};

struct PrimitiveDeclarator: public Declarator {
    PrimitiveDeclarator(Locatable loc, Symbol ident)
        : Declarator(loc, false)
        , _ident(ident) {};

    PrimitiveDeclarator(Locatable loc)
        : Declarator(loc, true)
        , _ident(nullptr) {};

    Symbol _ident;

    void print(std::ostream& stream);
};

struct FunctionDeclarator : public Declarator {
    FunctionDeclarator(Locatable loc, DeclaratorPtr decl)
        : Declarator(loc, decl->isAbstract())
        , _decl(std::move(decl)) {};

    DeclaratorPtr _decl;
    std::vector<Declaration> _parameters;

    void print(std::ostream& stream);
    void addParameter(Declaration param);
};

struct PointerDeclarator : public Declarator {
    PointerDeclarator(Locatable loc, DeclaratorPtr inner)
        : Declarator(loc, inner->isAbstract())
        , _inner(std::move(inner)) {};

    DeclaratorPtr _inner;

    void print(std::ostream& stream);
};

struct VoidSpecifier: public TypeSpecifier {
    public: 
    VoidSpecifier(const Locatable loc) : TypeSpecifier(loc) {};
    void print(std::ostream& stream);
};

struct CharSpecifier: public TypeSpecifier {
    public: 
    CharSpecifier(const Locatable loc) : TypeSpecifier(loc) {};
    void print(std::ostream& stream);
};

struct IntSpecifier: public TypeSpecifier {
    public: 
    IntSpecifier(const Locatable loc) : TypeSpecifier(loc) {};
    void print(std::ostream& stream);
};

struct StructSpecifier: public TypeSpecifier {
    public:
    StructSpecifier(const Locatable loc, Symbol tag) : TypeSpecifier(loc), _tag(tag) {};
    void print(std::ostream& stream);
    void addComponent(Declaration declaration);

    private:
    std::vector<Declaration> _components;
    Symbol _tag;
};
