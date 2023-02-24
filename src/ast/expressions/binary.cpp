#include "binary.h"

// BinaryExpression

void BinaryExpression::print(std::ostream& stream) {
    stream << '(' << this->_left << ' ' << this->_op_str << ' ' << this->_right << ')';
}

// MultiplyExpression

TypePtr MultiplyExpression::typecheck(ScopePtr& scope) {
    auto left_type = _left->typecheckWrap(scope);
    auto right_type = _right->typecheckWrap(scope);

    if (!left_type->isArithmetic() || !right_type->isArithmetic()) {
        errorloc(this->loc, "both sides of an arithmetic binary expression must be of arithemic type");
    }

    auto unified_type = unifyTypes(left_type, right_type);
    if (unified_type.has_value()) {
        this->type = unified_type.value();

        this->_left = castExpression(std::move(this->_left), unified_type.value());
        this->_right = castExpression(std::move(this->_right), unified_type.value());
    } else {
        this->type = INT_TYPE;
    }

    return this->type;
}

llvm::Value* MultiplyExpression::compileRValue(CompileScopePtr compile_scope) {
    llvm::Value* value_lhs = this->_left->compileRValue(compile_scope);
    llvm::Value* value_rhs = this->_right->compileRValue(compile_scope);
    return compile_scope->builder.CreateMul(
        compile_scope->builder.CreateIntCast(value_lhs, llvm::Type::getInt32Ty(compile_scope->ctx), true),
        compile_scope->builder.CreateIntCast(value_rhs, llvm::Type::getInt32Ty(compile_scope->ctx), true)
    );
}

// AddExpression

TypePtr AddExpression::typecheck(ScopePtr& scope) {
    auto left_type = _left->typecheckWrap(scope);
    auto right_type = _right->typecheckWrap(scope);

    if (left_type->isArithmetic() && right_type->isArithmetic()) {
        auto unified_type = unifyTypes(left_type, right_type);
        if (unified_type.has_value()) {
            this->type = unified_type.value();

            this->_left = castExpression(std::move(this->_left), unified_type.value());
            this->_right = castExpression(std::move(this->_right), unified_type.value());
        } else {
            this->type = INT_TYPE;
        }
        return this->type;
    }

    if (left_type->kind == TY_POINTER && right_type->isArithmetic()) {
        this->type = left_type;
        this->_right = castExpression(std::move(this->_right), INT_TYPE);

        return this->type;
    }

    if (left_type->isArithmetic() && right_type->kind == TY_POINTER) {
        this->type = right_type;
        this->_left = castExpression(std::move(this->_left), INT_TYPE);

        return this->type;
    }

    errorloc(this->loc, "Illegal addition operation");
}

llvm::Value* AddExpression::compileRValue(CompileScopePtr compile_scope) {
    llvm::Value* value_lhs = this->_left->compileRValue(compile_scope);
    llvm::Value* value_rhs = this->_right->compileRValue(compile_scope);

    // If one of them is pointer, get the pointer shifted by the value of the second operand
    if (this->_left->type->isPointer()) {
        auto ptr_type = std::static_pointer_cast<PointerType>(this->_left->type);
        auto inner_llvm_type = ptr_type->inner->toLLVMType(compile_scope);
        return compile_scope->builder.CreateInBoundsGEP(
            inner_llvm_type,
            value_lhs,
            compile_scope->builder.CreateIntCast(value_rhs, llvm::Type::getInt32Ty(compile_scope->ctx), false)
        );
    }

    if (this->_right->type->isPointer()) {
        auto ptr_type = std::static_pointer_cast<PointerType>(this->_right->type);
        auto inner_llvm_type = ptr_type->inner->toLLVMType(compile_scope);
        return compile_scope->builder.CreateInBoundsGEP(
            inner_llvm_type,
            value_rhs,
            compile_scope->builder.CreateIntCast(value_lhs, llvm::Type::getInt32Ty(compile_scope->ctx), false)
        );
    }

    return compile_scope->builder.CreateAdd(
        compile_scope->builder.CreateIntCast(value_lhs, llvm::Type::getInt32Ty(compile_scope->ctx), true),
        compile_scope->builder.CreateIntCast(value_rhs, llvm::Type::getInt32Ty(compile_scope->ctx), true)
    );
}

// SubtractExpression

TypePtr SubtractExpression::typecheck(ScopePtr& scope) {
    auto left_type = _left->typecheckWrap(scope);
    auto right_type = _right->typecheckWrap(scope);

    if (left_type->isArithmetic() && right_type->isArithmetic()) {
        auto unified_type = unifyTypes(left_type, right_type);
        if (unified_type.has_value()) {
            this->type = unified_type.value();

            this->_left = castExpression(std::move(this->_left), unified_type.value());
            this->_right = castExpression(std::move(this->_right), unified_type.value());
        } else {
            this->type = INT_TYPE;
        }

        return this->type;
    }
    if (left_type->kind == TY_POINTER && right_type->kind == TY_POINTER && left_type->equals(right_type)) {
        auto left_pointer = std::static_pointer_cast<PointerType>(left_type);
        auto right_pointer = std::static_pointer_cast<PointerType>(right_type);
        if (!(left_pointer->inner->isObjectType() && right_pointer->inner->isObjectType())) {
            errorloc(this->loc, "both pointers have to point to object types");
        }
        if (left_pointer->inner->kind == TY_STRUCT && right_pointer->inner->kind == TY_STRUCT) {
            auto left_struct = std::static_pointer_cast<StructType>(left_pointer->inner);
            auto right_struct = std::static_pointer_cast<StructType>(right_pointer->inner);
            if (!left_struct->isComplete() || !right_struct->isComplete()) {
                errorloc(this->loc, "both pointers have to point to object complete types");
            }
        }
        this->type = INT_TYPE;
        return this->type;
    }
    // left side must be complete object type
    if (left_type->kind == TY_POINTER && right_type->isArithmetic()) {
        auto pointer_type = std::static_pointer_cast<PointerType>(left_type);
        // not an object type
        if (!pointer_type->inner->isObjectType()) {
            errorloc(this->loc, "Illegal subtraction operation");
        }
        if (pointer_type->inner->kind == TY_STRUCT) {
            // not a complete object
            auto struct_type = std::static_pointer_cast<StructType>(pointer_type->inner);
            if (!struct_type->isComplete()) {
                errorloc(this->loc, "Illegal subtraction operation");
            }
        }
        this->type = left_type;
        return this->type;
    }

    errorloc(this->loc, "Illegal subtraction operation");
}

llvm::Value* SubtractExpression::compileRValue(CompileScopePtr compile_scope) {
    llvm::Value* value_lhs = this->_left->compileRValue(compile_scope);
    llvm::Value* value_rhs = this->_right->compileRValue(compile_scope);

    if (value_lhs->getType()->isPointerTy() && value_rhs->getType()->isPointerTy()) {
        auto type = this->_left->type;
        if (type->kind == TypeKind::TY_NULLPTR)
            type = this->_right->type;

        auto ptr_type = std::static_pointer_cast<PointerType>(type);
        auto inner_type = ptr_type->inner;
        auto inner_llvm_type = inner_type->toLLVMType(compile_scope);

        auto int_diff = compile_scope->builder.CreatePtrDiff(inner_llvm_type, value_lhs, value_rhs);
        return compile_scope->builder.CreateIntCast(int_diff, llvm::Type::getInt32Ty(compile_scope->ctx), false);
    }

    if (this->_left->type->isPointer()) {
        auto ptr_type = std::static_pointer_cast<PointerType>(this->_left->type);
        auto inner_llvm_type = ptr_type->inner->toLLVMType(compile_scope);
        return compile_scope->builder.CreateInBoundsGEP(
            inner_llvm_type,
            value_lhs,
            compile_scope->builder.CreateMul(
                compile_scope->builder.getInt32(-1),
                compile_scope->builder.CreateIntCast(value_rhs, llvm::Type::getInt32Ty(compile_scope->ctx), false)
            )
        );
    }

    return compile_scope->builder.CreateSub(
        compile_scope->builder.CreateIntCast(value_lhs, llvm::Type::getInt32Ty(compile_scope->ctx), true),
        compile_scope->builder.CreateIntCast(value_rhs, llvm::Type::getInt32Ty(compile_scope->ctx), true)
    );
}
// LessThanExpression

TypePtr LessThanExpression::typecheck(ScopePtr& scope) {
    auto left_type = this->_left->typecheckWrap(scope);
    auto right_type = this->_right->typecheckWrap(scope);

    this->type = INT_TYPE;

    auto unified_type = unifyTypes(left_type, right_type);
    if (!unified_type.has_value()) {
        if (!left_type->equals(right_type)) {
            errorloc(this->loc, "Cannot compare values of type ", left_type, " and ", right_type);
        }
        unified_type = left_type;
    }

    this->_left = castExpression(std::move(this->_left), unified_type.value());
    this->_right = castExpression(std::move(this->_right), unified_type.value());

    return this->type;
}

llvm::Value* LessThanExpression::compileRValue(CompileScopePtr compile_scope) {
    llvm::Value* value_lhs = this->_left->compileRValue(compile_scope);
    llvm::Value* value_rhs = this->_right->compileRValue(compile_scope);
    return compile_scope->builder.CreateICmpSLT(value_lhs, value_rhs);
}

// EqualExpression

TypePtr EqualExpression::typecheck(ScopePtr& scope) {
    auto left_type = this->_left->typecheckWrap(scope);
    auto right_type = this->_right->typecheckWrap(scope);

    this->type = INT_TYPE;

    auto unified_type = unifyTypes(left_type, right_type);
    if (!unified_type.has_value()) {
        if (!left_type->equals(right_type)) {
            errorloc(this->loc, "Cannot compare values of type ", left_type, " and ", right_type);
        }
        unified_type = left_type;
    }

    this->_left = castExpression(std::move(this->_left), unified_type.value());
    this->_right = castExpression(std::move(this->_right), unified_type.value());

    return this->type;
}

llvm::Value* EqualExpression::compileRValue(CompileScopePtr compile_scope) {
    llvm::Value* value_lhs = this->_left->compileRValue(compile_scope);
    llvm::Value* value_rhs = this->_right->compileRValue(compile_scope);
    return compile_scope->builder.CreateICmpEQ(value_lhs, value_rhs);
}

// UnequalExpression

TypePtr UnequalExpression::typecheck(ScopePtr& scope) {
    auto left_type = this->_left->typecheckWrap(scope);
    auto right_type = this->_right->typecheckWrap(scope);

    this->type = INT_TYPE;

    auto unified_type = unifyTypes(left_type, right_type);
    if (!unified_type.has_value()) {
        if (!left_type->equals(right_type)) {
            errorloc(this->loc, "Cannot compare values of type ", left_type, " and ", right_type);
        }
        unified_type = left_type;
    }

    this->_left = castExpression(std::move(this->_left), unified_type.value());
    this->_right = castExpression(std::move(this->_right), unified_type.value());

    return this->type;
}

llvm::Value* UnequalExpression::compileRValue(CompileScopePtr compile_scope) {
    llvm::Value* value_lhs = this->_left->compileRValue(compile_scope);
    llvm::Value* value_rhs = this->_right->compileRValue(compile_scope);
    return compile_scope->builder.CreateICmpNE(value_lhs, value_rhs);
}

// AndExpression

TypePtr AndExpression::typecheck(ScopePtr& scope) {
    auto left_type = this->_left->typecheckWrap(scope);
    auto right_type = this->_right->typecheckWrap(scope);
    if (!left_type->isScalar() || !right_type->isScalar()) {
        errorloc(this->loc, "Both sides of a logical and expression must be scalar types");
    }

    this->type = INT_TYPE;

    if (left_type->isPointer() && right_type->isPointer()) {
        return this->type;
    }

    auto unified_type = unifyTypes(left_type, right_type);
    if (!unified_type.has_value()) {
        if (!left_type->equals(right_type)) {
            errorloc(this->loc, "Cannot apply logical and operator to values of type ", left_type, " and ", right_type);
        }
        unified_type = left_type;
    }

    this->_left = castExpression(std::move(this->_left), unified_type.value());
    this->_right = castExpression(std::move(this->_right), unified_type.value());

    return this->type;
}

llvm::Value* AndExpression::compileRValue(CompileScopePtr compile_scope) {
    llvm::Value* value_lhs = toBoolTy(this->_left->compileRValue(compile_scope), compile_scope);
    auto lhs_block = compile_scope->builder.GetInsertBlock();

    // Add a basic block for the rhs expression
    llvm::BasicBlock* rhs_block = llvm::BasicBlock::Create(
        compile_scope->ctx,
        "and-consequence",
        compile_scope->function.value()
    );

    // Add a basic block for the end of the AndExpression
    llvm::BasicBlock* end_block = llvm::BasicBlock::Create(
        compile_scope->ctx,
        "and-end",
        compile_scope->function.value()
    );

    // If the lhs expr is true, we jump to the rhs expression, otherwise we skip to the end
    compile_scope->builder.CreateCondBr(value_lhs, rhs_block, end_block);

    // Compile the rhs expr
    compile_scope->builder.SetInsertPoint(rhs_block);
    llvm::Value* value_rhs = toBoolTy(this->_right->compileRValue(compile_scope), compile_scope);
    rhs_block = compile_scope->builder.GetInsertBlock();

    // Insert the jump to the end block
    compile_scope->builder.CreateBr(end_block);

    // Continue in the end block and combine the result of lhs and rhs
    compile_scope->builder.SetInsertPoint(end_block);

    llvm::PHINode* phi = compile_scope->builder.CreatePHI(compile_scope->builder.getInt1Ty(), 2);
    phi->addIncoming(value_lhs, lhs_block);
    phi->addIncoming(value_rhs, rhs_block);

    return compile_scope->builder.CreateIntCast(phi, llvm::Type::getInt32Ty(compile_scope->ctx), false);
}

// OrExpression

TypePtr OrExpression::typecheck(ScopePtr& scope) {
    auto left_type = this->_left->typecheckWrap(scope);
    auto right_type = this->_right->typecheckWrap(scope);
    if (!left_type->isScalar() || !right_type->isScalar()) {
        errorloc(this->loc, "Both sides of a logical or expression must be scalar types");
    }

    this->type = INT_TYPE;

    if (left_type->isPointer() && right_type->isPointer()) {
        return this->type;
    }

    auto unified_type = unifyTypes(left_type, right_type);
    if (!unified_type.has_value()) {
        if (!left_type->equals(right_type)) {
            errorloc(this->loc, "Cannot apply logical or operator to values of type ", left_type, " and ", right_type);
        }
        unified_type = left_type;
    }

    this->_left = castExpression(std::move(this->_left), unified_type.value());
    this->_right = castExpression(std::move(this->_right), unified_type.value());

    return this->type;
}

llvm::Value* OrExpression::compileRValue(CompileScopePtr compile_scope) {
    llvm::Value* value_lhs = toBoolTy(this->_left->compileRValue(compile_scope), compile_scope);
    auto lhs_block = compile_scope->builder.GetInsertBlock();

    // Add a basic block for the rhs expression
    llvm::BasicBlock* rhs_block = llvm::BasicBlock::Create(
        compile_scope->ctx,
        "or-consequence",
        compile_scope->function.value()
    );

    // Add a basic block for the end of the OrExpression
    llvm::BasicBlock* end_block = llvm::BasicBlock::Create(
        compile_scope->ctx,
        "or-end",
        compile_scope->function.value()
    );

    // If the lhs expr is true, we skip to the end, otherwise we jump to the rhs expr
    compile_scope->builder.CreateCondBr(value_lhs, end_block, rhs_block);

    // Compile the rhs expr
    compile_scope->builder.SetInsertPoint(rhs_block);
    llvm::Value* value_rhs = toBoolTy(this->_right->compileRValue(compile_scope), compile_scope);
    rhs_block = compile_scope->builder.GetInsertBlock();

    // Insert the jump to the end block
    compile_scope->builder.CreateBr(end_block);

    // Continue in the end block and combine the result of lhs and rhs
    compile_scope->builder.SetInsertPoint(end_block);

    llvm::PHINode* phi = compile_scope->builder.CreatePHI(compile_scope->builder.getInt1Ty(), 2);
    phi->addIncoming(value_lhs, lhs_block);
    phi->addIncoming(value_rhs, rhs_block);

    return compile_scope->builder.CreateIntCast(phi, llvm::Type::getInt32Ty(compile_scope->ctx), false);
}
// AssignExpression

TypePtr AssignExpression::typecheck(ScopePtr& scope) {
    auto right_type = this->_right->typecheckWrap(scope);
    auto left_type = this->_left->typecheckWrap(scope);

    // 6.5.16.0.2:
    // An assignment operator shall have a modifiable lvalue as its left operand.
    if (!this->_left->isLvalue(scope)) {
        errorloc(this->loc, "Can only assign to lvalues");
    }

    // 6.3.2.1.1:
    // A modifiable lvalue is an lvalue that [...] does not have an incomplete type
    if (!left_type->isComplete()) {
        errorloc(this->loc, "Cannot assign to an incomplete type");
    }

    // 6.5.16.0.3:
    // The type of an assignment expression is the type the left operand would have
    // after lvalue conversion.

    // 6.5.16.1.1:
    // One of the following shall hold:

    // the left operand has [...] arithmetic type,
    // and the right has arithmetic type;
    if (left_type->isArithmetic() && right_type->isArithmetic()) {
        this->type = left_type;

        if (!right_type->strong_equals(left_type)) {
            this->_right = castExpression(std::move(this->_right), left_type);
        }

        return this->type;
    }

    // the left operand has [...] structure or union type compatible with the type of the right;
    if (left_type->kind == TY_STRUCT) {
        if (left_type->equals(right_type)) {
            this->type = left_type;
            return this->type;
        }
        errorloc(this->loc, "left and right struct of an assign expression must be of compatible type");
    }

    // the left operand has [...] pointer type,
    // and (considering the type the left operand would have after lvalue conversion)
    // both operands are pointers to [...] compatible types [...];
    if (left_type->kind == TY_POINTER && right_type->kind == TY_POINTER) {
        if (left_type->equals(right_type)) {
            this->type = left_type;
            return this->type;
        }
    }

    // the left operand has [...] pointer type,
    // and (considering the type the left operand would have after lvalue conversion)
    // one operand is a pointer to an object type, and the other is a pointer to [...] void [...];
    if (left_type->kind == TY_POINTER && right_type->kind == TY_POINTER) {
        auto left = std::static_pointer_cast<PointerType>(left_type)->inner;
        auto right = std::static_pointer_cast<PointerType>(right_type)->inner;

        if ((left->isObjectType() && right->kind == TypeKind::TY_VOID)
            || (left->kind == TypeKind::TY_VOID && right->isObjectType())) {
            this->type = left_type;
            return this->type;
        }
    }

    // the left operand is a [...] pointer, and the right is a null pointer constant [...]
    if (left_type->kind == TY_POINTER && right_type->kind == TY_NULLPTR) {
        this->type = left_type;
        return this->type;
    }

    errorloc(this->loc, "wrong assign");
}

llvm::Value* AssignExpression::compileRValue(CompileScopePtr compile_scope) {
    llvm::Value* to_be_stored_in = this->_left->compileLValue(compile_scope);
    llvm::Value* value_to_be_stored = this->_right->compileRValue(compile_scope);
    compile_scope->builder.CreateStore(value_to_be_stored, to_be_stored_in);
    return value_to_be_stored;
}
