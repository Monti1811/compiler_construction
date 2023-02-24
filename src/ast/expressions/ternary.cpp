#include "ternary.h"

void TernaryExpression::print(std::ostream& stream) {
    stream << '(' << this->_condition << " ? " << this->_left << " : " << this->_right << ')';
}

TypePtr TernaryExpression::typecheck(ScopePtr& scope) {
    auto condition_type = this->_condition->typecheckWrap(scope);
    if (!condition_type->isScalar()) {
        errorloc(this->loc, "Condition type must be scalar");
    }

    auto left_type = this->_left->typecheckWrap(scope);
    auto right_type = this->_right->typecheckWrap(scope);

    if (left_type->kind == TypeKind::TY_VOID || right_type->kind == TypeKind::TY_VOID) {
        this->_left = castExpression(std::move(this->_left), VOID_TYPE);
        this->_right = castExpression(std::move(this->_right), VOID_TYPE);

        this->type = VOID_TYPE;
        return this->type;
    }

    auto unified_type = unifyTypes(left_type, right_type);
    if (!unified_type.has_value()) {
        if (!left_type->equals(right_type)) {
            errorloc(
                this->loc,
                "Second and third operand of ternary expression are incompatible; cannot unify ",
                left_type,
                " with ",
                right_type
            );
        }
        unified_type = left_type;
    }

    this->type = unified_type.value();

    this->_left = castExpression(std::move(this->_left), unified_type.value());
    this->_right = castExpression(std::move(this->_right), unified_type.value());

    return this->type;
}

llvm::Value* TernaryExpression::compileRValue(CompileScopePtr compile_scope) {
    auto condition_value = toBoolTy(this->_condition->compileRValue(compile_scope), compile_scope);

    // Add a basic block for the lhs expression
    llvm::BasicBlock* lhs_block = llvm::BasicBlock::Create(
        compile_scope->ctx,
        "ternary-lhs",
        compile_scope->function.value()
    );

    // Add a basic block for the rhs expression
    llvm::BasicBlock* rhs_block = llvm::BasicBlock::Create(
        compile_scope->ctx,
        "ternary-rhs",
        compile_scope->function.value()
    );

    // Add a basic block for the end of the TernaryExpression
    llvm::BasicBlock* end_block = llvm::BasicBlock::Create(
        compile_scope->ctx,
        "ternary-end",
        compile_scope->function.value()
    );

    // Create the conditional branch
    compile_scope->builder.CreateCondBr(condition_value, lhs_block, rhs_block);

    // Compile the lhs expression
    compile_scope->builder.SetInsertPoint(lhs_block);
    auto lhs_scope = std::make_shared<CompileScope>(compile_scope);
    auto lhs_value = this->_left->compileRValue(compile_scope);
    lhs_block = compile_scope->builder.GetInsertBlock();

    // Insert the jump to the end block
    compile_scope->builder.CreateBr(end_block);

    // Compile the rhs expression
    compile_scope->builder.SetInsertPoint(rhs_block);
    auto rhs_scope = std::make_shared<CompileScope>(compile_scope);
    auto rhs_value = this->_right->compileRValue(compile_scope);
    rhs_block = compile_scope->builder.GetInsertBlock();

    // Insert the jump to the end block
    compile_scope->builder.CreateBr(end_block);

    // Continue in the end block
    compile_scope->builder.SetInsertPoint(end_block);

    // If either side is void, we don't have anything to combine (both values are dropped)
    if (this->_left->type->kind == TypeKind::TY_VOID || this->_right->type->kind == TypeKind::TY_VOID) {
        return nullptr;
    }

    // Otherwise, combine the result of lhs and rhs
    llvm::PHINode* phi = compile_scope->builder.CreatePHI(lhs_value->getType(), 2);
    phi->addIncoming(lhs_value, lhs_block);
    phi->addIncoming(rhs_value, rhs_block);

    return phi;
}
