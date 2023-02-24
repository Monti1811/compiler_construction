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

llvm::FunctionType* FunctionType::toLLVMType(CompileScopePtr compile_scope) {
    if (this->has_params) {
        auto param_function_type = static_cast<ParamFunctionType*>(this);
        return param_function_type->toLLVMType(compile_scope);
    }

    /* Create the return type */
    llvm::Type* FuncReturnType = this->return_type->toLLVMType(compile_scope);

    /* Create a vector to store all parameter types */
    std::vector<llvm::Type*> FuncParamTypes;

    /* Create the (function) type of the function */
    return llvm::FunctionType::get(FuncReturnType, FuncParamTypes, /* isVarArg */ false);
}

bool ParamFunctionType::strong_equals(TypePtr const& other) {
    if (!FunctionType::strong_equals(other)) {
        return false;
    }

    auto other_type_func = std::static_pointer_cast<ParamFunctionType>(other);
    if (!other_type_func->has_params) {
        return this->params.size() == 0;
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

llvm::FunctionType* ParamFunctionType::toLLVMType(CompileScopePtr compile_scope) {
    /* Create the return type */
    llvm::Type* FuncReturnType = this->return_type->toLLVMType(compile_scope);

    /* Create a vector to store all parameter types */
    std::vector<llvm::Type*> FuncParamTypes;
    for (auto param : this->params) {
        if (param.type->kind == TypeKind::TY_FUNCTION) {
            FuncParamTypes.push_back(compile_scope->_Builder.getPtrTy());
        } else {
            llvm::Type* FuncParamType = param.type->toLLVMType(compile_scope);
            FuncParamTypes.push_back(FuncParamType);
        }
    }
    /* Create the (function) type of the function */
    return llvm::FunctionType::get(FuncReturnType, FuncParamTypes, /* isVarArg */ false);
}
