#include "expression.h"

#include "cast.h"

std::ostream& operator<<(std::ostream& stream, const ExpressionPtr& expr) {
    expr->print(stream);
    return stream;
}

TypePtr Expression::typecheckWrap(ScopePtr& scope) {
    auto type = this->typecheck(scope);
    if (type->kind == TypeKind::TY_FUNCTION) {
        return std::make_shared<PointerType>(type);
    }
    return type;
}

bool Expression::isLvalue(ScopePtr&) {
    return false;
}

llvm::Value* Expression::compileLValue(CompileScopePtr) {
    errorloc(this->loc, "Cannot compute lvalue of this expression");
}

std::optional<size_t> Expression::getStringLength(void) {
    return std::nullopt;
}

ExpressionPtr castExpression(ExpressionPtr expr, TypePtr type) {
    auto loc = expr->loc;
    auto cast = std::make_unique<CastExpression>(loc, std::move(expr));
    cast->type = type;
    return cast;
}

llvm::Value* toBoolTy(llvm::Value* to_convert, CompileScopePtr compile_scope) {
    if (to_convert->getType()->isIntegerTy(32)) {
        return compile_scope->builder.CreateICmpNE(compile_scope->builder.getInt32(0), to_convert);
    } else if (to_convert->getType()->isIntegerTy(8)) {
        return compile_scope->builder.CreateICmpNE(compile_scope->builder.getInt8(0), to_convert);
    } else if (to_convert->getType()->isPointerTy()) {
        // return 0 if nullptr, 1 otherwise
        llvm::Value* int_to_convert =
            compile_scope->builder.CreatePtrToInt(to_convert, compile_scope->builder.getInt32Ty());
        return compile_scope->builder.CreateICmpNE(compile_scope->builder.getInt32(0), int_to_convert);
    } else {
        return to_convert;
    }
}
