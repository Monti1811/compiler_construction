#include "suffix.h"

// IndexExpression

void IndexExpression::print(std::ostream& stream) {
    if (this->_swapped) {
        stream << '(' << this->_index << '[' << this->_expression << "])";
    } else {
        stream << '(' << this->_expression << '[' << this->_index << "])";
    }
}

TypePtr IndexExpression::typecheck(ScopePtr& scope) {
    auto expr_type = this->_expression->typecheckWrap(scope);
    auto index_type = this->_index->typecheckWrap(scope);

    TypePtr pointer_type;

    if (expr_type->isPointer() && index_type->isInteger()) {
        // Regular index expression: arr[0]
        pointer_type = expr_type;
        this->_index = castExpression(std::move(this->_index), INT_TYPE);
    } else if (expr_type->isInteger() && index_type->isPointer()) {
        // Swapped index expression: 0[arr]
        pointer_type = index_type;
        auto expr = std::move(this->_index);
        this->_index = castExpression(std::move(this->_expression), INT_TYPE);
        this->_expression = std::move(expr);
        this->_swapped = true;
    } else {
        errorloc(this->loc, "Index expressions must consist of a pointer and an integer");
    }

    TypePtr indexed_type;

    if (pointer_type->kind == TypeKind::TY_NULLPTR) {
        indexed_type = VOID_TYPE;
    } else {
        indexed_type = std::static_pointer_cast<PointerType>(pointer_type)->inner;
    }

    if (!indexed_type->isComplete() || !indexed_type->isObjectType()) {
        errorloc(this->loc, "Cannot index an incomplete or non-object type");
    }

    this->type = indexed_type;
    return this->type;
}

bool IndexExpression::isLvalue(ScopePtr&) {
    return true;
}

llvm::Value* IndexExpression::compileLValue(CompileScopePtr compile_scope) {
    // Get the type of this expression
    llvm::Type* index_type = this->type->toLLVMType(compile_scope);
    // Get the pointer of the array
    llvm::Value* array_alloca = this->_expression->compileRValue(compile_scope);
    // Get the value of the index
    llvm::Value* index_value = this->_index->compileRValue(compile_scope);
    // Get a pointer to the element at the index
    return compile_scope->builder.CreateInBoundsGEP(index_type, array_alloca, index_value);
}

llvm::Value* IndexExpression::compileRValue(CompileScopePtr compile_scope) {
    // Get the type of this expression
    llvm::Type* index_type = this->type->toLLVMType(compile_scope);
    llvm::Value* offset_ptr = this->compileLValue(compile_scope);
    // Load the value at the index
    return compile_scope->builder.CreateLoad(index_type, offset_ptr);
}

// CallExpression

void CallExpression::print(std::ostream& stream) {
    stream << '(' << this->_expression << '(';

    const int length = this->_arguments.size();
    for (int i = 0; i < length; i++) {
        this->_arguments.at(i)->print(stream);
        if (i != length - 1) {
            stream << ", ";
        }
    }

    stream << "))";
}

TypePtr CallExpression::typecheck(ScopePtr& scope) {
    auto expr_type = this->_expression->typecheckWrap(scope);

    auto function_type_opt = expr_type->unwrapFunctionPointer();
    if (!function_type_opt.has_value()) {
        errorloc(this->loc, "Call expression needs to be called on a function pointer");
    }
    auto function_type = function_type_opt.value();

    if (!function_type->return_type->isComplete() && function_type->return_type->kind != TypeKind::TY_VOID) {
        errorloc(this->loc, "Cannot call a function that returns a non-void incomplete type");
    }

    // A function without specified parameters can be called with any arguments
    if (!function_type->has_params) {
        for (auto& arg : this->_arguments) {
            arg->typecheckWrap(scope);
        }
        this->type = function_type->return_type;
        return this->type;
    }

    auto param_function_type = std::static_pointer_cast<ParamFunctionType>(function_type);
    auto params = param_function_type->params;

    if (this->_arguments.size() != params.size()) {
        errorloc(this->loc, "Incorrect number of arguments");
    }

    for (size_t i = 0; i < params.size(); i++) {
        auto arg_type = this->_arguments[i]->typecheckWrap(scope);
        auto unified_type = unifyTypes(arg_type, params[i].type);

        if (arg_type->equals(params[i].type) || unified_type.has_value()) {
            this->_arguments[i] = castExpression(std::move(this->_arguments[i]), params[i].type);
        } else if (params[i].type->kind == TY_FUNCTION && arg_type->kind == TY_POINTER) {
            // This case is OK, functions can be implicitly cast to function pointers
        } else {
            errorloc(this->_arguments[i]->loc, "Incorrect argument type, expected ", params[i].type, ", is ", arg_type);
        }
    }

    this->type = function_type->return_type;
    return this->type;
}

llvm::Value* CallExpression::compileRValue(CompileScopePtr compile_scope) {
    auto fun = this->_expression->compileRValue(compile_scope);

    auto function_type = this->_expression->type->unwrapFunctionPointer();
    if (!function_type.has_value()) {
        if (this->_expression->type->kind == TypeKind::TY_FUNCTION) {
            function_type = std::static_pointer_cast<FunctionType>(this->_expression->type);
        } else {
            errorloc(this->loc, "Could not unwrap function pointer during codegen");
        }
    }

    llvm::FunctionType* llvm_function_type = function_type.value()->toLLVMType(compile_scope);

    std::vector<llvm::Value*> args;
    for (auto& arg : this->_arguments) {
        llvm::Value* val = arg->compileRValue(compile_scope);
        args.push_back(val);
    }

    return compile_scope->builder.CreateCall(llvm_function_type, fun, llvm::makeArrayRef(args));
}

// DotExpression

void DotExpression::print(std::ostream& stream) {
    stream << '(' << this->_expression << '.' << *this->_ident << ')';
}

TypePtr DotExpression::typecheck(ScopePtr& scope) {
    auto expr_type = _expression->typecheckWrap(scope);
    if (expr_type->kind != TypeKind::TY_STRUCT) {
        errorloc(this->loc, "Cannot access a field of a non-struct expression");
    }
    if (!expr_type->isComplete()) {
        errorloc(this->loc, "Cannot access a field of an incomplete type");
    }
    auto struct_type = std::static_pointer_cast<CompleteStructType>(expr_type);

    auto field_type = struct_type->typeOfField(this->_ident);

    if (!field_type.has_value()) {
        errorloc(this->loc, "Field ", *this->_ident, " does not exist on ", expr_type);
    }
    this->type = field_type.value();
    return this->type;
}

bool DotExpression::isLvalue(ScopePtr& scope) {
    // 6.5.2.3.3:
    // [...] is an lvalue if the first expression is an lvalue.
    return this->_expression->isLvalue(scope);
}

llvm::Value* DotExpression::compileLValue(CompileScopePtr compile_scope) {
    auto expr_type = this->_expression->type;

    if (expr_type->kind != TY_STRUCT) {
        errorloc(this->loc, "Tried to access field of non-struct during codegen");
    }

    auto struct_ty = std::static_pointer_cast<StructType>(expr_type);
    if (!struct_ty->isComplete()) {
        errorloc(this->loc, "Tried to access field of incomplete struct during codegen");
    }

    auto complete_struct_ty = std::static_pointer_cast<CompleteStructType>(struct_ty);
    auto index = complete_struct_ty->getIndexOfField(this->_ident);

    std::vector<llvm::Value*> ElementIndexes;
    ElementIndexes.push_back(compile_scope->builder.getInt32(0));
    ElementIndexes.push_back(compile_scope->builder.getInt32(index));

    llvm::Type* struct_type = complete_struct_ty->toLLVMType(compile_scope);
    llvm::Value* struct_alloca = this->_expression->compileLValue(compile_scope);

    return compile_scope->builder.CreateInBoundsGEP(struct_type, struct_alloca, ElementIndexes);
}

llvm::Value* DotExpression::compileRValue(CompileScopePtr compile_scope) {
    llvm::Value* field_alloca = this->compileLValue(compile_scope);
    // Get type of the element
    llvm::Type* index_type = this->type->toLLVMType(compile_scope);
    return compile_scope->builder.CreateLoad(index_type, field_alloca);
}

// ArrowExpression

void ArrowExpression::print(std::ostream& stream) {
    stream << '(' << this->_expression << "->" << *this->_ident << ')';
}

TypePtr ArrowExpression::typecheck(ScopePtr& scope) {
    auto expr_type = _expression->typecheckWrap(scope);
    if (expr_type->kind != TypeKind::TY_POINTER) {
        errorloc(this->loc, "Cannot access non-pointers using the arrow operator");
    }
    auto pointer_type = std::static_pointer_cast<PointerType>(expr_type);

    if (pointer_type->inner->kind != TypeKind::TY_STRUCT) {
        errorloc(this->loc, "Cannot access a field of a non-struct expression");
    }
    auto inner_type = pointer_type->inner;
    // Check if the struct has a pointer to a struct as a field
    if (inner_type->kind == TY_STRUCT && !inner_type->isComplete()) {
        auto struct_type = std::static_pointer_cast<StructType>(inner_type);
        // If yes, check the scope to see if it was defined and use this definition to define the type
        std::optional<std::shared_ptr<CompleteStructType>> complete_type = scope->getCompleteStruct(*struct_type);
        if (complete_type.has_value()) {
            // Replace the struct type with the complete type
            pointer_type->inner = complete_type.value();
            // Replace the struct that should be used to calculate the type with the complete definition
            inner_type = complete_type.value();
        }
    }
    if (!inner_type->isComplete()) {
        errorloc(this->loc, "Cannot access a field of an incomplete type");
    }
    auto struct_type = std::static_pointer_cast<CompleteStructType>(inner_type);

    auto field_type = struct_type->typeOfField(this->_ident);

    if (!field_type.has_value()) {
        errorloc(this->loc, "Field ", *this->_ident, " does not exist on ", inner_type);
    }
    this->type = field_type.value();
    return this->type;
}

bool ArrowExpression::isLvalue(ScopePtr&) {
    // 6.5.2.3.4:
    // The value [...] is an lvalue.
    return true;
}

llvm::Value* ArrowExpression::compileLValue(CompileScopePtr compile_scope) {
    auto expr_type = this->_expression->type;

    if (expr_type->kind != TY_POINTER) {
        errorloc(this->loc, "Tried to access field of non-pointer during codegen");
    }

    auto struct_ptr_ty = std::static_pointer_cast<PointerType>(expr_type);
    if (struct_ptr_ty->inner->kind != TY_STRUCT) {
        errorloc(this->loc, "Tried to access field of non-struct during codegen");
    }
    auto struct_ty = std::static_pointer_cast<StructType>(struct_ptr_ty->inner);
    if (!struct_ty->isComplete()) {
        errorloc(this->loc, "Tried to access field of incomplete struct during codegen");
    }

    auto complete_struct_ty = std::static_pointer_cast<CompleteStructType>(struct_ty);
    auto index = complete_struct_ty->getIndexOfField(this->_ident);

    std::vector<llvm::Value*> ElementIndexes;
    ElementIndexes.push_back(compile_scope->builder.getInt32(0));
    ElementIndexes.push_back(compile_scope->builder.getInt32(index));

    llvm::Type* struct_type = complete_struct_ty->toLLVMType(compile_scope);
    llvm::Value* struct_alloca = this->_expression->compileRValue(compile_scope);
    // llvm::Value* struct_alloca = compile_scope->builder.CreateLoad(struct_type, struct_alloca_ptr);

    return compile_scope->builder.CreateInBoundsGEP(struct_type, struct_alloca, ElementIndexes);
}

llvm::Value* ArrowExpression::compileRValue(CompileScopePtr compile_scope) {
    llvm::Value* field_alloca = this->compileLValue(compile_scope);
    // Get type of the element
    llvm::Type* index_type = this->type->toLLVMType(compile_scope);
    return compile_scope->builder.CreateLoad(index_type, field_alloca);
}
