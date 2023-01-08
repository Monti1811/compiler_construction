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

    // Just for debugging purposes
    friend std::ostream& operator<<(std::ostream& stream, const std::shared_ptr<Type>& type);
    void print(std::ostream& stream);

    virtual bool equals(std::shared_ptr<Type> const& other);
    virtual bool strong_equals(std::shared_ptr<Type> const& other);

    bool isPointer();
    bool isInteger();
    bool isScalar();
    bool isArithmetic();
    bool isObjectType();
    virtual bool isComplete();

    // If this is a function pointer, extract the function type
    std::optional<std::shared_ptr<FunctionType>> getFunctionType();

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

    bool equals(TypePtr const& other) override;
    bool strong_equals(TypePtr const& other) override;

    TypePtr inner; 
};

static TypePtr STRING_TYPE = std::make_shared<PointerType>(CHAR_TYPE);

struct StructType: public Type {
    public:
    StructType(std::optional<Symbol> tag)
        : Type(TypeKind::TY_STRUCT)
        , tag(tag) {};

    bool equals(TypePtr const& other) override;
    bool strong_equals(TypePtr const& other) override;

    std::optional<Symbol> tag;
};

struct CompleteStructType: public StructType {
    public:
    CompleteStructType(std::optional<Symbol> tag)
        : StructType(tag) {};

    // TODO: Do we need to also consider fields in the equality check?
    // If so, we need to override equals() and maybe strong_equals() here.

    bool isComplete() override;

    // Returns true if field was already defined, false otherwise.
    bool addField(StructField field);

    // Adds all fields of another struct to this struct.
    // Returns true if any field was already defined, false otherwise.
    bool combineWith(CompleteStructType const& other);

    // Returns false if the constraints for named fields are not satisfied, true otherwise.
    bool validateFields();

    std::optional<TypePtr> typeOfField(Symbol& ident);

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
    
    bool equals(TypePtr const& other) override;
    bool strong_equals(TypePtr const& other) override;

    TypePtr return_type;
    bool has_params;
};

// This type describes a function that has params specified.
struct ParamFunctionType: FunctionType {
    ParamFunctionType(TypePtr const& return_type)
        : FunctionType(return_type, true) {};

    bool equals(TypePtr const& other) override;
    bool strong_equals(TypePtr const& other) override;

    void addParameter(FunctionParam const& param);

    std::vector<FunctionParam> params;
};
