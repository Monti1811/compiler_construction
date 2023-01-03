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
    TY_NULLPTR,
    TY_POINTER,
    TY_STRUCT,
    TY_FUNCTION,
};

struct Type {
    public:
    Type(const TypeKind kind)
        : kind(kind) {};

    virtual bool equals(std::shared_ptr<Type> const& other) {
        if (this->kind != other->kind) {
            if (
                (this->kind == TY_NULLPTR && other->kind == TY_INT) 
                || (this->kind == TY_INT && other->kind == TY_NULLPTR)
                || (this->kind == TY_NULLPTR && other->kind == TY_POINTER) 
                || (this->kind == TY_POINTER && other->kind == TY_NULLPTR)
                ) 
            {
                return true;
            }
            return false;
        }
        return true;
    }

    virtual bool strong_equals(std::shared_ptr<Type> const& other) {
        if (this->kind != other->kind) {
            return false;
        }
        return true;
    }

    bool isScalar() {
        switch (this->kind) {
            case TypeKind::TY_INT:
            case TypeKind::TY_CHAR:
            case TypeKind::TY_NULLPTR:
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
            case TypeKind::TY_NULLPTR:
                return true;
            default:
                return false;
        }
    }

    // Just for debugging purposes
    friend std::ostream& operator<<(std::ostream& stream, const std::shared_ptr<Type>& type);
    void print(std::ostream& stream);

    const TypeKind kind;
};

typedef std::shared_ptr<Type> TypePtr;

static TypePtr INT_TYPE = std::make_shared<Type>(TypeKind::TY_INT);
static TypePtr VOID_TYPE = std::make_shared<Type>(TypeKind::TY_VOID);
static TypePtr CHAR_TYPE = std::make_shared<Type>(TypeKind::TY_CHAR);
static TypePtr NULLPTR_TYPE = std::make_shared<Type>(TypeKind::TY_NULLPTR);

struct PointerType: public Type {
    public:
    PointerType(TypePtr inner)
        : Type(TypeKind::TY_POINTER)
        , inner(inner) {};

    bool equals(TypePtr const& other) {
        // return true when other or this pointer is a nullptr
        if (other->kind == TypeKind::TY_NULLPTR || inner->kind == TypeKind::TY_VOID) {
            return true;
        }
        if (other->kind == TypeKind::TY_POINTER) {
            auto other_pointer = std::static_pointer_cast<PointerType>(other);
            if (other_pointer->inner->kind == TY_VOID) {
                return true;
            }
            return this->inner->equals(other_pointer->inner);
        } else {
            return false;
        }
    }

    bool strong_equals(TypePtr const& other) {
        if (other->kind != TypeKind::TY_POINTER) {
            return false;
        }
        auto other_pointer = std::static_pointer_cast<PointerType>(other);
        return this->inner->strong_equals(other_pointer->inner);
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

    bool equals(TypePtr const& other) {
        if (other->kind != TypeKind::TY_STRUCT) {
            return false;
        }
        auto other_structtype = std::static_pointer_cast<StructType>(other);
        // TODO: Not sure if it is enough to check only for the name 
        // or if fields should also be checked
        if (other_structtype->_tag != this->_tag) {
            return false;
        }
        return true;
    }

    bool strong_equals(TypePtr const& other) {
        return this->equals(other);
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

    bool equals(TypePtr const& other) {
        if (other->kind != TypeKind::TY_FUNCTION) {
            return false;
        }
        auto other_functiontype = std::static_pointer_cast<FunctionType>(other);
        if (!(this->return_type->strong_equals(other_functiontype->return_type))) {
            return false;
        }

        if (this->args.size() != other_functiontype->args.size()) {
            return false;
        }
        for (long long unsigned int i = 0; i < this->args.size(); i++) {
            auto arg1 = this->args[i];
            auto arg2 = other_functiontype->args[i];
            if (!(arg1->strong_equals(arg2))) {
                return false;
            }
        }
        return true;
    }

    bool strong_equals(TypePtr const& other) {
        return this->equals(other);
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
    std::unordered_map<Symbol, TypePtr> concrete_fndef;
    std::unordered_map<Symbol, std::shared_ptr<StructType>> structs;

    std::optional<TypePtr> getTypeVar(Symbol ident) {
        if (vars.find(ident) == vars.end()) {
            if (!parent.has_value()) {
                return std::nullopt;
            }
            return parent.value()->getTypeVar(ident);
        }
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
        return !(this->vars.insert({ name, type }).second);
    }
    
    // Returns whether the function was already defined
    bool addFunctionDeclaration(Symbol name, TypePtr const& type) {
        auto def_type = this->vars.find(name);
        // Check if types are the same
        if (def_type != this->vars.end() && !(def_type->second->equals(type))) {
            return true;
        }
        bool success = this->concrete_fndef.insert({ name, type }).second;
        this->vars.insert({ name, type });
        return !success;
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

