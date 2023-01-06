#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "../util/diagnostic.h"
#include "../util/symbol_internalizer.h"

#include "types.h"
#include "type_decl.h"

enum DeclaratorKind {
    PRIMITIVE,
    FUNCTION,
    POINTER
};

enum SpecifierKind {
    VOID,
    INT,
    CHAR,
    STRUCT
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

    virtual std::optional<Symbol> getName() = 0;
    virtual TypePtr wrapType(TypePtr const&, ScopePtr&) = 0;

    const Locatable loc;
    const DeclaratorKind kind;

    private:
    bool _abstract = false;
};

typedef std::unique_ptr<Declarator> DeclaratorPtr;

struct TypeSpecifier {
    public:
    TypeSpecifier(const Locatable loc, const SpecifierKind kind) : _loc(loc), _kind(kind) {};
    virtual ~TypeSpecifier() = default;

    virtual void print(std::ostream& stream) = 0;
    friend std::ostream& operator<<(std::ostream& stream, const std::unique_ptr<TypeSpecifier>& type);

    virtual TypePtr toType(ScopePtr& scope) = 0;


    protected:
    const Locatable _loc;

    public:
    SpecifierKind _kind;
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
    TypeDecl toType(ScopePtr& scope);
    void checkDefinition(ScopePtr& scope);

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
        , _ident(std::nullopt) {};

    void print(std::ostream& stream);

    std::optional<Symbol> getName() {
        return this->_ident;
    }
    TypePtr wrapType(TypePtr const& type, ScopePtr&) {
        return type;
    }
    bool isAbstract() {
        return !this->_ident.has_value();
    }

    std::optional<Symbol> _ident;
};

struct FunctionDeclarator : public Declarator {
    FunctionDeclarator(Locatable loc, DeclaratorPtr decl)
        : Declarator(loc, decl->isAbstract(), DeclaratorKind::FUNCTION)
        , _name(std::move(decl)) {};

    DeclaratorPtr _name;
    std::vector<Declaration> _parameters;

    void print(std::ostream& stream);
    void addParameter(Declaration param);

    std::optional<Symbol> getName() {
        return this->_name->getName();
    }

    TypePtr wrapType(TypePtr const& type, ScopePtr& scope) {
        auto functionPointer = [](TypePtr func_type) {
            return std::make_shared<PointerType>(func_type);
        };

        if (this->_parameters.empty()) {
            auto function_type = std::make_shared<FunctionType>(type);
            return functionPointer(function_type);
        }

        auto function_type = std::make_shared<ParamFunctionType>(type);

        if (this->_parameters.size() == 1) {
            auto param = this->_parameters[0].toType(scope);

            if (param.type->kind == TypeKind::TY_VOID) {
                if (!param.isAbstract()) {
                    errorloc(this->loc, "void function parameter must be abstract");
                }
                return functionPointer(function_type);
            }
        }

        for (auto& param_decl : this->_parameters) {
            auto param = param_decl.toType(scope);

            if (param.type->kind == TypeKind::TY_VOID) {
                errorloc(this->loc, "function parameters cannot be void, unless void is the only parameter");
            }
            function_type->addParameter(param);
        }

        return functionPointer(function_type);
    }
};

struct PointerDeclarator : public Declarator {
    PointerDeclarator(Locatable loc, DeclaratorPtr inner)
        : Declarator(loc, inner->isAbstract(), DeclaratorKind::POINTER)
        , _inner(std::move(inner)) {};

    DeclaratorPtr _inner;

    void print(std::ostream& stream);

    std::optional<Symbol> getName() {
        return this->_inner->getName();
    }
    TypePtr wrapType(TypePtr const& type, ScopePtr& scope) {
        auto wrapped = std::make_shared<PointerType>(type);
        return this->_inner->wrapType(wrapped, scope);
    }
};

struct VoidSpecifier: public TypeSpecifier {
    public: 
    VoidSpecifier(const Locatable loc) : TypeSpecifier(loc, SpecifierKind::VOID) {};

    void print(std::ostream& stream);

    TypePtr toType(ScopePtr&) { return VOID_TYPE; }
};

struct CharSpecifier: public TypeSpecifier {
    public: 
    CharSpecifier(const Locatable loc) : TypeSpecifier(loc, SpecifierKind::CHAR) {};

    void print(std::ostream& stream);
    TypePtr toType(ScopePtr&) { return CHAR_TYPE; }
};

struct IntSpecifier: public TypeSpecifier {
    public: 
    IntSpecifier(const Locatable loc) : TypeSpecifier(loc, SpecifierKind::INT) {};

    void print(std::ostream& stream);

    TypePtr toType(ScopePtr&) { return INT_TYPE; }
};

struct StructSpecifier: public TypeSpecifier {
    public:
    StructSpecifier(const Locatable loc, std::optional<Symbol> tag) : 
        TypeSpecifier(loc, SpecifierKind::STRUCT), _tag(tag) {};

    void print(std::ostream& stream);

    void makeComplete();
    void addComponent(Declaration declaration);

    TypePtr toType(ScopePtr&);

    std::optional<Symbol> _tag;

    private:
    std::optional<std::vector<Declaration>> _components;
};
