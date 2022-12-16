#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "../util/diagnostic.h"
#include "../util/symbol_internalizer.h"

#include "types.h"

enum DeclaratorKind {
    PRIMITIVE,
    FUNCTION,
    POINTER
};

struct Declarator {
    public:
    Declarator(const Locatable loc, const bool abstract, const DeclaratorKind kind)
        : loc(loc)
        , kind(kind)
        , _abstract(abstract) {};

    virtual void print(std::ostream& stream) = 0;
    friend std::ostream& operator<<(std::ostream& stream, const std::unique_ptr<Declarator>& declarator);

    bool isAbstract();

    virtual Symbol getName() = 0;
    virtual TypePtr wrapType(TypePtr const&, ScopePtr&) = 0;

    const Locatable loc;
    const DeclaratorKind kind;

    private:
    bool _abstract = false;
};

typedef std::unique_ptr<Declarator> DeclaratorPtr;

struct TypeSpecifier {
    public:
    TypeSpecifier(const Locatable loc) : _loc(loc) {};
    virtual ~TypeSpecifier() = default;

    virtual void print(std::ostream& stream) = 0;
    friend std::ostream& operator<<(std::ostream& stream, const std::unique_ptr<TypeSpecifier>& type);

    virtual TypePtr toType(ScopePtr& scope) = 0;

    protected:
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

    void typecheck(ScopePtr& scope);
    std::pair<Symbol, TypePtr> toType(ScopePtr& scope);

    Locatable _loc;
    TypeSpecifierPtr _specifier;
    DeclaratorPtr _declarator;
};

struct PrimitiveDeclarator: public Declarator {
    PrimitiveDeclarator(Locatable loc, Symbol ident)
        : Declarator(loc, false, DeclaratorKind::PRIMITIVE)
        , _ident(ident) {};

    PrimitiveDeclarator(Locatable loc)
        : Declarator(loc, true, DeclaratorKind::PRIMITIVE)
        , _ident(nullptr) {};

    Symbol _ident;

    void print(std::ostream& stream);

    Symbol getName() {
        return this->_ident;
    }
    TypePtr wrapType(TypePtr const& type, ScopePtr&) {
        return type;
    }
};

struct FunctionDeclarator : public Declarator {
    FunctionDeclarator(Locatable loc, DeclaratorPtr decl)
        : Declarator(loc, decl->isAbstract(), DeclaratorKind::FUNCTION)
        , _name(std::move(decl)) {};

    DeclaratorPtr _name;
    std::vector<Declaration> _parameters;

    void print(std::ostream& stream);
    void addParameter(Declaration param);

    Symbol getName() {
        return this->_name->getName();
    }
    TypePtr wrapType(TypePtr const& type, ScopePtr& scope) {
        auto function_type = std::make_shared<FunctionType>(type);
        for (auto& arg : this->_parameters) {
            auto pair = arg.toType(scope);
            function_type->addArgument(pair.second);
        }
        return std::make_shared<PointerType>(function_type);
    }
};

struct PointerDeclarator : public Declarator {
    PointerDeclarator(Locatable loc, DeclaratorPtr inner)
        : Declarator(loc, inner->isAbstract(), DeclaratorKind::POINTER)
        , _inner(std::move(inner)) {};

    DeclaratorPtr _inner;

    void print(std::ostream& stream);

    Symbol getName() {
        return this->_inner->getName();
    }
    TypePtr wrapType(TypePtr const& type, ScopePtr&) {
        return std::make_shared<PointerType>(type);
    }
};

struct VoidSpecifier: public TypeSpecifier {
    public: 
    VoidSpecifier(const Locatable loc) : TypeSpecifier(loc) {};

    void print(std::ostream& stream);

    TypePtr toType(ScopePtr&) { return VOID_TYPE; }
};

struct CharSpecifier: public TypeSpecifier {
    public: 
    CharSpecifier(const Locatable loc) : TypeSpecifier(loc) {};

    void print(std::ostream& stream);
    TypePtr toType(ScopePtr&) { return CHAR_TYPE; }
};

struct IntSpecifier: public TypeSpecifier {
    public: 
    IntSpecifier(const Locatable loc) : TypeSpecifier(loc) {};

    void print(std::ostream& stream);

    TypePtr toType(ScopePtr&) { return INT_TYPE; }
};

struct StructSpecifier: public TypeSpecifier {
    public:
    StructSpecifier(const Locatable loc, std::optional<Symbol> tag) : TypeSpecifier(loc), _tag(tag) {};

    void print(std::ostream& stream);

    void addComponent(Declaration declaration);
    TypePtr toType(ScopePtr&);

    private:
    std::vector<Declaration> _components;
    std::optional<Symbol> _tag;
};
