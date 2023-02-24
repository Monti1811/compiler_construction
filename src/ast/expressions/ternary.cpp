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
    /* Add a basic block for the consequence of the TernaryExpression */
    llvm::BasicBlock* TernaryConsequenceBlock = llvm::BasicBlock::Create(
        compile_scope->ctx /* LLVMContext &Context */,
        "ternary-consequence" /* const Twine &Name="" */,
        compile_scope->function.value() /* Function *Parent=0 */,
        0 /* BasicBlock *InsertBefore=0 */
    );

    /* Add a basic block for the alternative of the TernaryExpression */
    llvm::BasicBlock* TernaryAlternativeBlock = llvm::BasicBlock::Create(
        compile_scope->ctx /* LLVMContext &Context */,
        "ternary-alternative" /* const Twine &Name="" */,
        compile_scope->function.value() /* Function *Parent=0 */,
        0 /* BasicBlock *InsertBefore=0 */
    );
    /* Add a basic block for the end of the TernaryExpression (after the Ternary) */
    llvm::BasicBlock* TernaryEndBlock = llvm::BasicBlock::Create(
        compile_scope->ctx /* LLVMContext &Context */,
        "ternary-end" /* const Twine &Name="" */,
        compile_scope->function.value() /* Function *Parent=0 */,
        0 /* BasicBlock *InsertBefore=0 */
    );
    /* Create the conditional branch */
    compile_scope->builder.CreateCondBr(condition_value, TernaryConsequenceBlock, TernaryAlternativeBlock);
    /* Set the header of the TernaryConsequenceBlock as the new insert point */
    compile_scope->builder.SetInsertPoint(TernaryConsequenceBlock);
    auto consequence_compile_scope = std::make_shared<CompileScope>(compile_scope);
    auto true_value = this->_left->compileRValue(compile_scope);
    auto true_block = compile_scope->builder.GetInsertBlock();
    /* Insert the jump to the Ternary end block */
    compile_scope->builder.CreateBr(TernaryEndBlock);
    /* Set the header of the TernaryAlternativeBlock as the new insert point */
    compile_scope->builder.SetInsertPoint(TernaryAlternativeBlock);
    auto alternative_compile_scope = std::make_shared<CompileScope>(compile_scope);
    auto false_value = this->_right->compileRValue(compile_scope);
    auto false_block = compile_scope->builder.GetInsertBlock();
    /* Insert the jump to the Ternary end block */
    compile_scope->builder.CreateBr(TernaryEndBlock);
    /* Continue in the Ternary end block */
    compile_scope->builder.SetInsertPoint(TernaryEndBlock);

    if (this->_left->type->kind == TypeKind::TY_VOID || this->_right->type->kind == TypeKind::TY_VOID) {
        return nullptr;
    }
    llvm::PHINode* phi = compile_scope->builder.CreatePHI(true_value->getType(), 2);
    phi->addIncoming(true_value, true_block);
    phi->addIncoming(false_value, false_block);
    return phi;
}
