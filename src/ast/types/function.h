#pragma once

#include "type.h"

#include "../scope.h"

// This type describes a function without any parameters specified.
struct FunctionType: public Type {
    public:
    FunctionType(TypePtr const& return_type, ScopePtr scope, bool has_params = false)
        : Type(TypeKind::TY_FUNCTION)
        , return_type(return_type)
        , scope(scope)
        , has_params(has_params) {
            this->scope->function_return_type = return_type;
        };
    
    bool equals(TypePtr const& other) override;
    bool strong_equals(TypePtr const& other) override;
    llvm::FunctionType* toLLVMType(llvm::IRBuilder<>& Builder, llvm::LLVMContext& Ctx);

    TypePtr return_type;
    ScopePtr scope;
    bool has_params;
};

// This type describes a function that has params specified.
struct ParamFunctionType: public FunctionType {
    public:
    ParamFunctionType(TypePtr const& return_type, ScopePtr scope)
        : FunctionType(return_type, scope, true) {};

    bool strong_equals(TypePtr const& other) override;

    void addParameter(FunctionParam const& param);

    llvm::FunctionType* toLLVMType(llvm::IRBuilder<>& Builder, llvm::LLVMContext& Ctx);

    std::vector<FunctionParam> params;
};
