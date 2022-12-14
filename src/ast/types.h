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

    static std::unique_ptr<Type> makeInt() {
        return std::make_unique<Type>(TypeKind::TY_INT);
    }
    static std::unique_ptr<Type> makeVoid() {
        return std::make_unique<Type>(TypeKind::TY_VOID);
    }
    static std::unique_ptr<Type> makeChar() {
        return std::make_unique<Type>(TypeKind::TY_CHAR);
    }

    virtual bool equals(std::unique_ptr<Type>& other) {
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

typedef std::unique_ptr<Type> TypePtr;

struct PointerType: public Type {
    public:
    PointerType(TypePtr inner)
        : Type(TypeKind::TY_POINTER)
        , inner(std::move(inner)) {};

    bool equals(TypePtr& other) {
        if (other->kind == TypeKind::TY_POINTER) {
            auto other_pointer = static_cast<PointerType>(std::move(other));
            return this->inner->equals(other_pointer.inner);
        } else {
            return false;
        }
    }

    TypePtr inner; 
};

struct StructType: public Type {
    public:
    StructType()
        : Type(TypeKind::TY_STRUCT) {};

    void addField(Symbol name, TypePtr type) {
        this->fields.insert({ name, std::move(type) });
    }

    bool equals(TypePtr&) {
        // TODO?
        return false;
    }

    std::unordered_map<Symbol, TypePtr> fields;
};

struct FunctionType: public Type {
    public:
    FunctionType(TypePtr return_type)
        : Type(TypeKind::TY_FUNCTION)
        , return_type(std::move(return_type)) {};
    
    void addArgument(TypePtr type) {
        this->args.push_back(std::move(type));
    }

    bool equals(TypePtr&) {
        return false; // TODO
    }

    TypePtr return_type;
    std::vector<TypePtr> args;
};

struct Scope {
    // TODO: Remember to put function pointers in here as well
    std::unordered_map<Symbol, TypePtr> vars;
    std::unordered_map<Symbol, StructType> structs;

    std::optional<TypePtr> getTypeVar(Symbol ident) {
        if (vars.find(ident) == vars.end()) {
            if (!parent) {
                return std::nullopt;
            }
            return parent->getTypeVar(ident);
        }
        return std::move(vars.at(ident));
    }

    std::optional<StructType> getTypeStruct(Symbol ident) {
        if (structs.find(ident) == structs.end()) {
            if (!parent) {
                return std::nullopt;
            }
            return parent->getTypeStruct(ident);
        }
        return std::move(structs.at(ident));
    }

    void addDeclaration(Symbol name, TypePtr type) {
        this->vars.insert({ name, std::move(type) });
    }

    // TODO: Vars & things in parent scope also present in child scope
    std::unique_ptr<Scope> parent;
};

typedef std::unique_ptr<Scope> ScopePtr;

