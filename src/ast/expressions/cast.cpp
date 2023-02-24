#include "cast.h"

void CastExpression::print(std::ostream& stream) {
    stream << this->_inner;
}

TypePtr CastExpression::typecheck(ScopePtr&) {
    // Cast Expression does not exist during typecheck phase, so just return type that is being cast to
    return this->type;
}

llvm::Value* CastExpression::compileLValue(CompileScopePtr compile_scope) {
    auto converted_expr = this->convertNullptrs(compile_scope);
    if (converted_expr.has_value()) {
        return converted_expr.value();
    }

    auto value = this->_inner->compileLValue(compile_scope);
    return this->castArithmetics(compile_scope, value);
}

llvm::Value* CastExpression::compileRValue(CompileScopePtr compile_scope) {
    auto converted_expr = this->convertNullptrs(compile_scope);
    if (converted_expr.has_value()) {
        return converted_expr.value();
    }

    auto value = this->_inner->compileRValue(compile_scope);
    return this->castArithmetics(compile_scope, value);
}

std::optional<llvm::Value*> CastExpression::convertNullptrs(CompileScopePtr compile_scope) {
    if (this->_inner->type->kind != TypeKind::TY_NULLPTR) {
        return std::nullopt;
    }

    if (this->type->kind == TypeKind::TY_INT) {
        return compile_scope->builder.getInt32(0);
    } else if (this->type->kind == TypeKind::TY_CHAR) {
        return compile_scope->builder.getInt8(0);
    } else if (this->type->kind == TypeKind::TY_NULLPTR) {
        return std::nullopt;
    } else if (this->type->kind == TypeKind::TY_POINTER) {
        return std::nullopt;
    } else if (this->type->kind == TypeKind::TY_VOID) {
        return std::nullopt;
    } else {
        errorloc(this->loc, "Invalid usage of null pointer constant");
    }
}

llvm::Value* CastExpression::castArithmetics(CompileScopePtr compile_scope, llvm::Value* value) {
    auto from_type = this->_inner->type;
    auto to_type = this->type;

    if (from_type->equals(to_type)) {
        return value;
    }

    auto llvm_type = to_type->toLLVMType(compile_scope);

    if ((from_type->kind == TypeKind::TY_CHAR && to_type->isInteger())
        || (from_type->isInteger() && to_type->kind == TypeKind::TY_CHAR)) {
        return compile_scope->builder.CreateIntCast(value, llvm_type, true);
    }

    if (from_type->isInteger() && to_type->isPointer()) {
        return compile_scope->builder.CreateIntToPtr(value, llvm_type);
    } else if (from_type->isPointer() && to_type->isInteger()) {
        return compile_scope->builder.CreatePtrToInt(value, llvm_type);
    }

    return value;
}
