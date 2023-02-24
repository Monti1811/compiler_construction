#include "unary.h"

#include "primary.h"

// UnaryExpression

void UnaryExpression::print(std::ostream& stream) {
    stream << '(' << this->_op_str << this->_inner << ')';
}

// SizeofExpression

TypePtr SizeofExpression::typecheck(ScopePtr& scope) {
    auto inner_type = this->_inner->typecheck(scope);

    if (inner_type->kind == TY_FUNCTION) {
        errorloc(this->_inner->loc, "inner of a sizeof expression must not have function type");
    }
    if (inner_type->kind == TY_STRUCT) {
        auto struct_type = std::static_pointer_cast<StructType>(inner_type);
        if (struct_type->tag.has_value()) {
            // We retrieve the struct again from the scope, because we might have gotten an incomplete struct
            // that was redefined at some other point
            auto type = scope->getStructType(struct_type->tag.value());
            if (type.has_value()) {
                struct_type = type.value();
            }
        }
        if (!struct_type->isComplete()) {
            errorloc(this->_inner->loc, "inner of sizeof expression must not have incomplete type");
        }
    }
    this->type = INT_TYPE;
    return this->type;
}

llvm::Value* SizeofExpression::compileRValue(CompileScopePtr compile_scope) {
    auto string_length = this->_inner->getStringLength();
    if (this->_inner->type->isString() && string_length.has_value()) {
        // String literals and expressions like *&"foo"
        return compile_scope->builder.getInt32(string_length.value());
    }
    auto inner_type = this->_inner->type->toLLVMType(compile_scope);
    return compile_scope->builder.getInt32(compile_scope->module.getDataLayout().getTypeAllocSize(inner_type));
}

// SizeofTypeExpression

void SizeofTypeExpression::print(std::ostream& stream) {
    stream << "(sizeof(" << this->_decl << "))";
}

TypePtr SizeofTypeExpression::typecheck(ScopePtr& scope) {
    this->_inner_type = this->_decl.toType(scope).type;
    this->type = INT_TYPE;
    return this->type;
}

llvm::Value* SizeofTypeExpression::compileRValue(CompileScopePtr compile_scope) {
    auto llvm_type = this->_inner_type->toLLVMType(compile_scope);
    return compile_scope->builder.getInt32(compile_scope->module.getDataLayout().getTypeAllocSize(llvm_type));
}

// ReferenceExpression

TypePtr ReferenceExpression::typecheck(ScopePtr& scope) {
    auto inner_type = this->_inner->typecheck(scope);

    // 6.5.3.2.1:
    // The operand of the unary & operator shall be either a function designator,
    // the result of a [] or unary * operator, or an lvalue

    // IndexExpression and DerefExpression are always lvalues
    if (this->_inner->isLvalue(scope) || inner_type->kind == TypeKind::TY_FUNCTION) {
        this->type = std::make_shared<PointerType>(inner_type);
        return this->type;
    }

    errorloc(this->loc, "expression to be referenced must be a function designator or an lvalue");
}

llvm::Value* ReferenceExpression::compileRValue(CompileScopePtr compile_scope) {
    return this->_inner->compileLValue(compile_scope);
}

std::optional<size_t> ReferenceExpression::getStringLength(void) {
    auto inner = dynamic_cast<StringLiteralExpression*>(this->_inner.get());
    if (!inner) {
        return std::nullopt;
    }
    return this->_inner->getStringLength();
}

// DerefExpression

TypePtr DerefExpression::typecheck(ScopePtr& scope) {
    auto inner_type = this->_inner->typecheckWrap(scope);
    if (inner_type->kind != TypeKind::TY_POINTER) {
        errorloc(this->loc, "Cannot dereference a non-pointer");
    }
    auto pointer_type = std::static_pointer_cast<PointerType>(inner_type);
    this->type = pointer_type->inner;
    return this->type;
}

bool DerefExpression::isLvalue(ScopePtr&) {
    // 6.5.3.2.4:
    // If the operand points to a function, the result is a function designator;
    // if it points to an object, the result is an lvalue designating the object.

    // NOTE: Always returning true here technically isn't quite correct.
    // However, it should not make a noticeable difference,
    // because AssignExpression throws an error when it encounters a function type.
    return true;
}

llvm::Value* DerefExpression::compileLValue(CompileScopePtr compile_scope) {
    return this->_inner->compileRValue(compile_scope);
}

llvm::Value* DerefExpression::compileRValue(CompileScopePtr compile_scope) {
    auto expr_value = this->compileLValue(compile_scope);
    auto expr_type = this->_inner->type;

    // If we try to dereference a function, don't do anything and just return the function
    if (expr_type->kind == TypeKind::TY_FUNCTION) {
        return expr_value;
    }

    if (expr_type->kind != TypeKind::TY_POINTER) {
        errorloc(this->loc, "Tried to dereference an expression of type ", expr_type, " during codegen");
    }

    auto ptr_ty = std::static_pointer_cast<PointerType>(expr_type);

    // If we try to dereference a function, don't do anything and just return the function
    if (ptr_ty->inner->kind == TypeKind::TY_FUNCTION) {
        return expr_value;
    }

    auto inner_expr_type = ptr_ty->inner->toLLVMType(compile_scope);
    return compile_scope->builder.CreateLoad(inner_expr_type, expr_value);
}

std::optional<size_t> DerefExpression::getStringLength(void) {
    auto inner = dynamic_cast<ReferenceExpression*>(this->_inner.get());
    if (!inner) {
        return std::nullopt;
    }
    return this->_inner->getStringLength();
}

// NegationExpression

TypePtr NegationExpression::typecheck(ScopePtr& scope) {
    auto inner_type = this->_inner->typecheckWrap(scope);
    if (!inner_type->isArithmetic()) {
        errorloc(this->loc, "type to be negated has to be arithmetic");
    }
    this->type = INT_TYPE;
    return this->type;
}

llvm::Value* NegationExpression::compileRValue(CompileScopePtr compile_scope) {
    llvm::Value* inner_value = this->_inner->compileRValue(compile_scope);
    inner_value = compile_scope->builder.CreateIntCast(inner_value, llvm::Type::getInt32Ty(compile_scope->ctx), true);
    return compile_scope->builder.CreateMul(compile_scope->builder.getInt32(-1), inner_value);
}

// LogicalNegationExpression

TypePtr LogicalNegationExpression::typecheck(ScopePtr& scope) {
    auto inner_type = _inner->typecheckWrap(scope);
    if (!inner_type->isScalar()) {
        errorloc(this->loc, "type to be logcially negated has to be scalar");
    }
    this->type = INT_TYPE;
    return this->type;
}

llvm::Value* LogicalNegationExpression::compileRValue(CompileScopePtr compile_scope) {
    llvm::Value* inner_value = toBoolTy(this->_inner->compileRValue(compile_scope), compile_scope);
    auto result = compile_scope->builder.CreateICmpEQ(compile_scope->builder.getInt1(0), inner_value);
    return compile_scope->builder.CreateIntCast(result, llvm::Type::getInt32Ty(compile_scope->ctx), false);
}
