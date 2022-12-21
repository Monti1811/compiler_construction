#pragma once

#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>
#include <iostream>

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

    bool isArithmetic() {
        switch(this->kind) {
            case TypeKind::TY_INT:
            case TypeKind::TY_CHAR:
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

    bool addField(Symbol name, TypePtr const& type) {
        return !this->fields.insert({ name, type }).second;
    }

    bool equals(TypePtr const&) {
        // TODO?
        return false;
    }

    void setAnonymous(bool anon) {
        anonymous = anon;
    }

    bool isAnonymous() {
        return anonymous;
    }

    void setTag(Symbol tag) {
        this->_tag = tag;
    }

    std::unordered_map<Symbol, TypePtr> fields;
    bool anonymous;
    Symbol _tag;
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
        : parent(parent)
        , functionReturnType(parent->functionReturnType)
        , loop_counter(parent->loop_counter) {};
    Scope(std::shared_ptr<Scope> parent, std::unordered_set<Symbol> labels)
        : parent(parent)
        , labels(labels)
        , functionReturnType(parent->functionReturnType)
        , loop_counter(parent->loop_counter) {};

    // TODO: Remember to put function pointers in here as well
    std::unordered_map<Symbol, TypePtr> vars;
    std::unordered_map<Symbol, std::shared_ptr<StructType>> structs;

    std::optional<TypePtr> getTypeVar(Symbol ident) {
        if (vars.find(ident) == vars.end()) {
            if (!parent.has_value()) {
                return std::nullopt;
            }
            return parent.value()->getTypeVar(ident);
        }
        /* TypePtr ret = vars.at(ident);
        if (ret->kind == TY_STRUCT) {
            auto struct_type = std::static_pointer_cast<StructType>(ret);
            auto tag = struct_type->_tag;
            auto full_struct_type = getTypeStruct(tag);
            return full_struct_type;
        } */
        return vars.at(ident);
    }

    std::optional<std::shared_ptr<StructType>> getTypeStruct(Symbol ident) {
        if (structs.find(ident) == structs.end()) {
            if (!parent.has_value()) {
                return std::nullopt;
            }
            return parent.value()->getTypeStruct(ident);
        }
        return structs.at(ident);
    }

    std::optional<TypePtr> getFunctionReturnType() {
        return functionReturnType;
    }

    bool isLabelDefined(Symbol label) {
        if (this->labels.find(label) == this->labels.end()) {
            if (!parent.has_value()) {
                return false;
            }
            return parent.value()->isLabelDefined(label);
        }
        return true;
    }

    // Returns whether the variable was already defined
    bool addDeclaration(Symbol name, TypePtr const& type) {
        if (type->kind == TY_STRUCT) {
            auto struct_type = std::static_pointer_cast<StructType>(type);
            auto tag = struct_type->_tag;
            auto full_struct_type = getTypeStruct(tag);
            if (full_struct_type.has_value()) {
                return !(this->vars.insert({ name, full_struct_type.value() }).second);
            } 
            return !(this->vars.insert({ name, struct_type }).second);
        }
        return !(this->vars.insert({ name, type }).second);
    }

    // Returns whether the struct was already defined
    bool addStruct(Symbol name, std::shared_ptr<StructType> type) {
        return !(this->structs.insert({ name, type }).second);
    }

    // adds the function return type
    void addFunctionReturnType(TypePtr returnType) {
        this->functionReturnType = returnType;
    }

    std::optional<std::shared_ptr<Scope>> parent;

    std::unordered_set<Symbol> labels;
    
    // used to typecheck a return-statement
    std::optional<TypePtr> functionReturnType;

    int loop_counter = 0;
};

typedef std::shared_ptr<Scope> ScopePtr;

