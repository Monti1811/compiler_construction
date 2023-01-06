#pragma once

#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>
#include <iostream>

#include "../util/symbol_internalizer.h"

#include "type_decl.h"

enum TypeKind {
    TY_INT,
    TY_VOID,
    TY_CHAR,
    TY_NULLPTR,
    TY_POINTER,
    TY_STRUCT,
    TY_FUNCTION,
};

struct FunctionType;

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
        switch (this->kind) {
            case TypeKind::TY_INT:
            case TypeKind::TY_CHAR:
            case TypeKind::TY_NULLPTR:
                return true;
            default:
                return false;
        }
    }

    bool isObjectType() {
        switch (this->kind)
        {
            case TypeKind::TY_FUNCTION:
            case TypeKind::TY_VOID:
                return false;
            default:
                return true;
        }
    }

    virtual bool isComplete() {
        switch (this->kind) {
            case TypeKind::TY_VOID:
            case TypeKind::TY_STRUCT:
                return false;
            default:
                return true;
        }
    }

    // If this is a function pointer, extract the function type
    std::optional<std::shared_ptr<FunctionType>> getFunctionType();

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
    StructType(std::optional<Symbol> tag)
        : Type(TypeKind::TY_STRUCT)
        , tag(tag) {};

    bool equals(TypePtr const& other) {
        if (other->kind != TypeKind::TY_STRUCT) {
            return false;
        }
        auto other_structtype = std::static_pointer_cast<StructType>(other);

        if (other_structtype->tag != this->tag) {
            return false;
        }
        return true;
    }

    bool strong_equals(TypePtr const& other) {
        return this->equals(other);
    }

    std::optional<Symbol> tag;
};

struct CompleteStructType: public StructType {
    public:
    CompleteStructType(std::optional<Symbol> tag)
        : StructType(tag) {};

    // TODO: Do we need to also consider fields in the equality check?
    // If so, we need to override equals() and maybe strong_equals() here.

    bool isComplete() {
        return true;
    }

    // Returns true if field was already defined, false otherwise.
    bool addField(StructField field) {
        if (field.name.has_value() && !this->_field_names.insert({ field.name.value(), this->fields.size() }).second) {
            return true;
        }

        this->fields.push_back(field);
        return false;
    }

    // Adds all fields of another struct to this struct.
    // Returns true if any field was already defined, false otherwise.
    bool combineWith(CompleteStructType const& other) {
        for (auto& field : other.fields) {
            if (this->addField(field)) {
                return true;
            }
        }
        return false;
    }

    // Returns false if the constraints for named fields are not satisfied, true otherwise.
    bool validateFields() {
        // 6.7.2.1.8:
        // If the struct-declaration-list does not contain any
        // named members, either directly or via an anonymous structure or anonymous union, the
        // behavior is undefined.
        // (we throw an error because why not)

        if (this->_field_names.empty()) {
            return false;
        }

        // 6.7.2.1.15:
        // There may be unnamed padding within a structure object, but not at its beginning.
        if (this->fields.at(0).isAbstract()) {
            return false;
        } else {
            return true;
        }
    }

    std::optional<TypePtr> typeOfField(Symbol& ident) {
        if (this->_field_names.find(ident) == this->_field_names.end()) {
            return std::nullopt;
        }
        return this->fields.at(this->_field_names.at(ident)).type;
    }

    std::vector<StructField> fields;

    private:
    std::unordered_map<Symbol, size_t> _field_names;
};

// This type describes a function without any parameters specified.
struct FunctionType: public Type {
    public:
    FunctionType(TypePtr const& return_type, bool has_params = false)
        : Type(TypeKind::TY_FUNCTION)
        , return_type(return_type)
        , has_params(has_params) {};
    
    bool equals(TypePtr const& other) {
        if (other->kind != TypeKind::TY_FUNCTION) {
            return false;
        }

        auto other_type = std::static_pointer_cast<FunctionType>(other);
        return this->return_type->strong_equals(other_type->return_type);
    }

    bool strong_equals(TypePtr const& other) {
        return this->equals(other);
    }

    TypePtr return_type;
    bool has_params;
};

// This type describes a function that has params specified.
struct ParamFunctionType: FunctionType {
    ParamFunctionType(TypePtr const& return_type)
        : FunctionType(return_type, true) {};

    void addParameter(FunctionParam const& param) {
        this->params.push_back(param);
    }

    bool equals(TypePtr const& other) {
        if (!FunctionType::equals(other)) {
            return false;
        }

        auto other_type = std::static_pointer_cast<ParamFunctionType>(other);

        if (this->params.size() != other_type->params.size()) {
            return false;
        }

        for (size_t i = 0; i < this->params.size(); i++) {
            auto param1 = this->params[i];
            auto param2 = other_type->params[i];
            if (!(param1.type->strong_equals(param2.type))) {
                return false;
            }
        }
        return true;
    }

    bool strong_equals(TypePtr const& other) {
        return this->equals(other);
    }

    std::vector<FunctionParam> params;
};

struct Scope {
    Scope() : parent(std::nullopt) {};
    Scope(std::shared_ptr<Scope> parent)
        : parent(parent)
        , function_return_type(parent->function_return_type)
        , loop_counter(parent->loop_counter) {};
    Scope(std::shared_ptr<Scope> parent, std::unordered_set<Symbol> labels)
        : parent(parent)
        , labels(labels)
        , function_return_type(parent->function_return_type)
        , loop_counter(parent->loop_counter) {};

    std::optional<TypePtr> getVarType(Symbol ident) {
        if (this->vars.find(ident) == this->vars.end()) {
            if (!this->parent.has_value()) {
                return std::nullopt;
            }
            return this->parent.value()->getVarType(ident);
        }
        return this->vars.at(ident);
    }

    std::optional<std::shared_ptr<StructType>> getStructType(Symbol ident) {
        if (this->structs.find(ident) == this->structs.end()) {
            if (!this->parent.has_value()) {
                return std::nullopt;
            }
            return this->parent.value()->getStructType(ident);
        }
        return this->structs.at(ident);
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

    std::optional<TypePtr> getFunctionReturnType() {
        return this->function_return_type;
    }

    // Returns whether the variable was already defined
    bool addDeclaration(TypeDecl& decl) {
        if (decl.isAbstract()) {
            return false;
        }

        return !(this->vars.insert({ decl.name.value(), decl.type }).second);
    }
    
    // Returns whether the function was already defined
    bool addFunctionDeclaration(TypeDecl& decl) {
        if (decl.isAbstract()) {
            return false;
        }
        auto name = decl.name.value();

        auto decl_type = this->vars.find(name);
        // If function was already declared, check if types are the same
        if (decl_type != this->vars.end() && !(decl_type->second->equals(decl.type))) {
            return true;
        }
        // Add the function declaration to the scope's variables
        this->vars.insert({ name, decl.type });
        // Mark this function as defined - if it was already before, return true.
        bool function_newly_defined = this->defined_functions.insert(name).second;
        return !function_newly_defined;
    }

    // Returns whether the struct was already defined
    bool addStruct(std::shared_ptr<StructType> type) {
        // TODO:
        // 6.7.2.1.8:
        // The presence of a struct-declaration-list in a struct-or-union-specifier declares a new type,
        // *within a translation unit*

        if (!type->tag.has_value()) {
            return false;
        }
        auto name = type->tag.value();

        auto scope_struct = this->structs.find(name);

        if (scope_struct != this->structs.end() && scope_struct->second->isComplete()) {
            // The struct has already been completed in this scope
            return true;
        }

        this->structs.insert({ name, type });
        return false;
    }

    // adds the function return type
    void setFunctionReturnType(TypePtr return_type) {
        this->function_return_type = return_type;
    }

    std::optional<std::shared_ptr<Scope>> parent;

    std::unordered_map<Symbol, TypePtr> vars;
    std::unordered_set<Symbol> defined_functions;

    std::unordered_map<Symbol, std::shared_ptr<StructType>> structs;

    std::unordered_set<Symbol> labels;
    
    // used to typecheck a return-statement
    std::optional<TypePtr> function_return_type;

    int loop_counter = 0;
};

typedef std::shared_ptr<Scope> ScopePtr;

