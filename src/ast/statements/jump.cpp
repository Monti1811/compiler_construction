#include "jump.h"

// JumpStatement

void JumpStatement::print(std::ostream& stream) {
    stream << this->_jump_str << ';';
}

// GotoStatement

void GotoStatement::print(std::ostream& stream) {
    stream << "goto " << *this->_ident << ';';
}

void GotoStatement::typecheck(ScopePtr& scope) {
    if (this->_ident->length() == 0) {
        errorloc(this->loc, "Labels cannot be empty");
    }
    if (!scope->isLabelDefined(this->_ident)) {
        errorloc(this->loc, "Missing label");
    }
}

void GotoStatement::compile(CompileScopePtr compile_scope) {
    // Retrieve the labeled block
    std::optional<llvm::BasicBlock*> labled_block = compile_scope->getLabeledBlock(this->_ident);
    if (labled_block.has_value()) {
        compile_scope->builder.CreateBr(labled_block.value());
    }

    // Create an extra dead block that extra code after the goto statement is added to
    llvm::BasicBlock* return_dead_block = llvm::BasicBlock::Create(
        compile_scope->ctx,
        "DEAD_BLOCK",
        compile_scope->function.value()
    );
    compile_scope->builder.SetInsertPoint(return_dead_block);
}

// ContinueStatement

void ContinueStatement::typecheck(ScopePtr& scope) {
    if (scope->loop_counter == 0) {
        errorloc(this->loc, "Invalid 'continue' outside of a loop");
    }
}

void ContinueStatement::compile(CompileScopePtr compile_scope) {
    // Retrieve the block to continue at
    std::optional<llvm::BasicBlock*> continue_block = compile_scope->getContinueBlock();
    if (continue_block.has_value()) {
        compile_scope->builder.CreateBr(continue_block.value());
    }

    // Create an extra dead block that extra code after the continue statement is added to
    llvm::BasicBlock* return_dead_block = llvm::BasicBlock::Create(
        compile_scope->ctx,
        "DEAD_BLOCK",
        compile_scope->function.value()
    );
    compile_scope->builder.SetInsertPoint(return_dead_block);
}

// BreakStatement

void BreakStatement::typecheck(ScopePtr& scope) {
    if (scope->loop_counter == 0) {
        errorloc(this->loc, "Invalid 'break' outside of a loop");
    }
}

void BreakStatement::compile(CompileScopePtr compile_scope) {
    // Retrieve the block to break to
    std::optional<llvm::BasicBlock*> break_block = compile_scope->getBreakBlock();
    if (break_block.has_value()) {
        compile_scope->builder.CreateBr(break_block.value());
    }

    // Create an extra dead block that extra code after the break statement is added to
    llvm::BasicBlock* return_dead_block = llvm::BasicBlock::Create(
        compile_scope->ctx,
        "DEAD_BLOCK",
        compile_scope->function.value()
    );
    compile_scope->builder.SetInsertPoint(return_dead_block);
}

// ReturnStatement

void ReturnStatement::print(std::ostream& stream) {
    stream << "return";
    if (this->_expr) {
        stream << " " << this->_expr.value();
    }
    stream << ';';
}

void ReturnStatement::typecheck(ScopePtr& scope) {
    auto return_type_opt = scope->function_return_type;
    if (!return_type_opt.has_value()) {
        errorloc(this->loc, "Return Statement in a non-function block");
    }

    auto return_type = return_type_opt.value();

    if (return_type->kind == TypeKind::TY_VOID) {
        if (_expr.has_value()) {
            errorloc(this->loc, "return statement must be empty if return type is void");
        }
        return;
    }

    if (!_expr.has_value()) {
        errorloc(this->loc, "expected a return expression but got none");
    }

    auto expr_type = _expr.value()->typecheckWrap(scope);

    auto unified_type = unifyTypes(return_type, expr_type);
    if (!unified_type.has_value() && !expr_type->equals(return_type)) {
        errorloc(this->loc, "return type and type of return expr did not match");
    }

    this->_expr.value() = castExpression(std::move(this->_expr.value()), return_type);
}

void ReturnStatement::compile(CompileScopePtr compile_scope) {
    if (this->_expr.has_value()) {
        llvm::Value* return_value = this->_expr.value()->compileRValue(compile_scope);

        // If return type is a bool, cast it to int32
        if (return_value->getType()->isIntegerTy(1)) {
            auto int_type = llvm::Type::getInt32Ty(compile_scope->ctx);
            return_value = compile_scope->builder.CreateIntCast(return_value, int_type, false);
        }

        compile_scope->builder.CreateRet(return_value);
    } else {
        compile_scope->builder.CreateRetVoid();
    }

    // Create an extra dead block that extra code after the return statement is added to
    llvm::BasicBlock* return_dead_block = llvm::BasicBlock::Create(
        compile_scope->ctx,
        "DEAD_BLOCK",
        compile_scope->function.value()
    );
    compile_scope->builder.SetInsertPoint(return_dead_block);
}
