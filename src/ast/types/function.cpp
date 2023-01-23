#include "function.h"

bool FunctionType::equals(TypePtr const& other) {
    return other->kind == TypeKind::TY_FUNCTION;
}

bool FunctionType::strong_equals(TypePtr const& other) {
    if (!FunctionType::equals(other)) {
        return false;
    }

    auto other_type = std::static_pointer_cast<FunctionType>(other);
    return this->return_type->strong_equals(other_type->return_type);
}

llvm::FunctionType* FunctionType::toLLVMType(llvm::IRBuilder<>& Builder, llvm::LLVMContext& Ctx) {
    /* Create the return type */
    llvm::Type *FuncReturnType = this->return_type->toLLVMType(Builder, Ctx);

    /* Create a vector to store all parameter types */
    std::vector<llvm::Type *> FuncParamTypes;

    /* Create the (function) type of the function */
    return llvm::FunctionType::get(
        FuncReturnType, FuncParamTypes, /* isVarArg */ false);
}

bool ParamFunctionType::strong_equals(TypePtr const& other) {
    if (!FunctionType::strong_equals(other)) {
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

void ParamFunctionType::addParameter(FunctionParam const& param) {
    this->params.push_back(param);
}

llvm::FunctionType* ParamFunctionType::toLLVMType(llvm::IRBuilder<>& Builder, llvm::LLVMContext& Ctx) {
    /* Create the return type */
    llvm::Type *FuncReturnType = this->return_type->toLLVMType(Builder, Ctx);

    /* Create a vector to store all parameter types */
    std::vector<llvm::Type *> FuncParamTypes;
    for (auto param : this->params) {
        llvm::Type *FuncParamType = param.type->toLLVMType(Builder, Ctx);
        FuncParamTypes.push_back(FuncParamType);
    }
    /* Create the (function) type of the function */
    return llvm::FunctionType::get(
        FuncReturnType, FuncParamTypes, /* isVarArg */ false);
}
