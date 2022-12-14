#pragma once

#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

#include "../util/symbol_internalizer.h"

enum TypeKind {
    TY_INT,
    TY_VOID,
    TY_CHAR,
    TY_POINTER,
    TY_STRUCT,
    TY_FUNCTION,
};

struct Type {
    public:
    Type(const TypeKind kind)
        : kind(kind) {};

    virtual bool equals(std::shared_ptr<Type> const& other) {
        return this->kind == other->kind;
    }

    bool isScalar() {
        switch (this->kind) {
            case TypeKind::TY_INT:
            case TypeKind::TY_CHAR:
            case TypeKind::TY_POINTER:
                return true;
            default:
                return false;
        }
    }

    const TypeKind kind;
};

typedef std::shared_ptr<Type> TypePtr;

static TypePtr INT_TYPE = std::make_shared<Type>(TypeKind::TY_INT);
static TypePtr VOID_TYPE = std::make_shared<Type>(TypeKind::TY_VOID);
static TypePtr CHAR_TYPE = std::make_shared<Type>(TypeKind::TY_CHAR);

struct PointerType: public Type {
    public:
    PointerType(TypePtr inner)
        : Type(TypeKind::TY_POINTER)
        , inner(inner) {};

    bool equals(TypePtr const& other) {
        if (other->kind == TypeKind::TY_POINTER) {
            auto other_pointer = std::static_pointer_cast<PointerType>(other);
            return this->inner->equals(other_pointer->inner);
        } else {
            return false;
        }
    }

    TypePtr inner; 
};

static TypePtr STRING_TYPE = std::make_shared<PointerType>(CHAR_TYPE);

struct StructType: public Type {
    public:
    StructType()
        : Type(TypeKind::TY_STRUCT) {};

    void addField(Symbol name, TypePtr const& type) {
        this->fields.insert({ name, type });
    }

    bool equals(TypePtr const&) {
        // TODO?
        return false;
    }

    std::unordered_map<Symbol, TypePtr> fields;
};

struct FunctionType: public Type {
    public:
    FunctionType(TypePtr const& return_type)
        : Type(TypeKind::TY_FUNCTION)
        , return_type(return_type) {};
    
    void addArgument(TypePtr const& type) {
        this->args.push_back(type);
    }

    bool equals(TypePtr const&) {
        return false; // TODO
    }

    TypePtr return_type;
    std::vector<TypePtr> args;
};

struct Scope {
    Scope() : parent(std::nullopt) {};
    Scope(std::shared_ptr<Scope> parent)
        : parent(parent) {};

    // TODO: Remember to put function pointers in here as well
    std::unordered_map<Symbol, TypePtr> vars;
    std::unordered_map<Symbol, StructType> structs;

    std::optional<TypePtr> getTypeVar(Symbol ident) {
        if (vars.find(ident) == vars.end()) {
            if (!parent.has_value()) {
                return std::nullopt;
            }
            return parent.value()->getTypeVar(ident);
        }
        return vars.at(ident);
    }

    std::optional<StructType> getTypeStruct(Symbol ident) {
        if (structs.find(ident) == structs.end()) {
            if (!parent.has_value()) {
                return std::nullopt;
            }
            return parent.value()->getTypeStruct(ident);
        }
        return structs.at(ident);
    }

    void addDeclaration(Symbol name, TypePtr const& type) {
        this->vars.insert({ name, type });
    }

    // TODO: Vars & things in parent scope also present in child scope
    std::optional<std::shared_ptr<Scope>> parent;
};

typedef std::shared_ptr<Scope> ScopePtr;

