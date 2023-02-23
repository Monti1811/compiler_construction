#pragma once

#include <iostream>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

#include "../../llvm.h"
#include "../../util/symbol_internalizer.h"

#include "../type_decl.h"

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
        : kind(kind){};

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
    bool isString();
    virtual bool isComplete();
    llvm::Type* toLLVMType(llvm::IRBuilder<>& Builder, llvm::LLVMContext& Ctx);

    // If this is a function pointer, extract the function type
    std::optional<std::shared_ptr<FunctionType>> unwrapFunctionPointer();

    const TypeKind kind;
};

typedef std::shared_ptr<Type> TypePtr;

static TypePtr INT_TYPE = std::make_shared<Type>(TypeKind::TY_INT);
static TypePtr VOID_TYPE = std::make_shared<Type>(TypeKind::TY_VOID);
static TypePtr CHAR_TYPE = std::make_shared<Type>(TypeKind::TY_CHAR);
static TypePtr NULLPTR_TYPE = std::make_shared<Type>(TypeKind::TY_NULLPTR);
