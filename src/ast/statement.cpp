#include "statement.h"

#include "indentation.h"
#include "specifiers/struct.h"

std::ostream& operator<<(std::ostream& stream, const StatementPtr& stat) {
    stat->print(stream);
    return stream;
}

void LabeledStatement::print(std::ostream& stream) {
    IndentManager& indent = IndentManager::getInstance();
    stream << *this->_name << ':';
    if (this->_inner.get()->kind == StatementKind::ST_LABELED) {
        stream << '\n' << this->_inner;
    } else {
        stream << '\n' << indent << this->_inner;
    }
}

void LabeledStatement::typecheck(ScopePtr& scope) {
    this->_inner->typecheck(scope);
}

void LabeledStatement::compile(CompileScopePtr compile_scope) {
    auto labeledBlockOpt = compile_scope->getLabeledBlock(this->_name);
    if (!labeledBlockOpt.has_value()) {
        errorloc(this->loc, "Unknown label ", *this->_name);
    }
    auto labeledBlock = labeledBlockOpt.value();
    compile_scope->builder.CreateBr(labeledBlock);
    compile_scope->builder.SetInsertPoint(labeledBlock);
    auto inner_compile_scope = std::make_shared<CompileScope>(compile_scope);
    this->_inner->compile(inner_compile_scope);
}

void BlockStatement::print(std::ostream& stream) {
    stream << "{";
    IndentManager& indent = IndentManager::getInstance();
    indent.increaseCurrIndentation(1);
    for (auto& item : this->_items) {
        if (item.get()->kind == StatementKind::ST_LABELED) {
            stream << '\n' << item;
        } else {
            stream << '\n' << indent << item;
        }
    }
    indent.decreaseCurrIndentation(1);
    stream << '\n' << indent << '}';
}

void BlockStatement::typecheck(ScopePtr& scope) {
    auto block_scope = std::make_shared<Scope>(scope);
    this->typecheckInner(block_scope);
}

void BlockStatement::typecheckInner(ScopePtr& inner_scope) {
    for (auto& item : this->_items) {
        item->typecheck(inner_scope);
    }
}

void BlockStatement::compile(CompileScopePtr compile_scope) {
    auto inner_compile_scope = std::make_shared<CompileScope>(compile_scope);

    // First compile all declarations, then compile other statements
    for (auto& item : this->_items) {
        if (item->kind == StatementKind::ST_DECLARATION) {
            item->compile(inner_compile_scope);
        }
    }
    for (auto& item : this->_items) {
        if (!(item->kind == StatementKind::ST_DECLARATION)) {
            item->compile(inner_compile_scope);
        }
    }
}

void EmptyStatement::print(std::ostream& stream) {
    stream << ';';
}

void EmptyStatement::compile(CompileScopePtr) {}

void DeclarationStatement::print(std::ostream& stream) {
    stream << this->_declaration << ';';
}

void DeclarationStatement::typecheck(ScopePtr& scope) {
    this->_declaration.typecheck(scope);
}

void DeclarationStatement::compile(CompileScopePtr compile_scope) {
    TypePtr type_dcl = this->_declaration.getTypeDecl().type;
    auto name = this->_declaration.getTypeDecl().name;
    // do nothing if it is abstract
    if (name.has_value()) {

        llvm::Type* llvm_type = type_dcl->toLLVMType(compile_scope);
        /* Allocate stack space for the variable */
        llvm::Value* LocalVarPtr = compile_scope->alloca_builder.CreateAlloca(llvm_type);
        /* Reset the alloca builder each time before using it */
        compile_scope->alloca_builder.SetInsertPoint(
            compile_scope->alloca_builder.GetInsertBlock(), compile_scope->alloca_builder.GetInsertBlock()->begin()
        );
        // fill compilescope
        compile_scope->addAlloca(name.value(), LocalVarPtr);
        compile_scope->addType(name.value(), llvm_type);
    }
}

void ExpressionStatement::print(std::ostream& stream) {
    stream << this->_expr << ';';
}

void ExpressionStatement::typecheck(ScopePtr& scope) {
    this->_expr->typecheck(scope);
}

void ExpressionStatement::compile(CompileScopePtr compile_scope) {
    this->_expr->compileRValue(compile_scope);
}

void IfStatement::print(std::ostream& stream) {
    stream << "if (" << this->_condition << ')';
    IndentManager& indent = IndentManager::getInstance();
    bool hasElseStatement = this->_else_statement.has_value();

    if (this->_then_statement->kind == StatementKind::ST_BLOCK) {
        stream << ' ' << this->_then_statement;
        if (hasElseStatement) {
            stream << " ";
        }
    } else {
        indent.increaseCurrIndentation(1);
        stream << "\n" << indent << this->_then_statement;
        indent.decreaseCurrIndentation(1);
        if (hasElseStatement) {
            stream << "\n" << indent;
        }
    }

    if (hasElseStatement) {
        auto& else_statement = this->_else_statement.value();
        auto else_type = else_statement->kind;

        if (else_type == StatementKind::ST_BLOCK || else_type == StatementKind::ST_IF) {
            stream << "else " << else_statement;
        } else {
            stream << "else";
            indent.increaseCurrIndentation(1);
            stream << "\n" << indent << else_statement;
            indent.decreaseCurrIndentation(1);
        }
    }
}

void IfStatement::typecheck(ScopePtr& scope) {
    auto condition_type = this->_condition->typecheckWrap(scope);
    if (!condition_type->isScalar()) {
        errorloc(this->_condition->loc, "Condition of an if statement must be scalar");
    }
    this->_then_statement->typecheck(scope);
    if (this->_else_statement.has_value()) {
        this->_else_statement->get()->typecheck(scope);
    }
}

void IfStatement::compile(CompileScopePtr compile_scope) {
    /* Add a basic block for the header of the IfStmt */
    llvm::BasicBlock* IfHeaderBlock = llvm::BasicBlock::Create(
        compile_scope->ctx /* LLVMContext &Context */,
        "if-header" /* const Twine &Name="" */,
        compile_scope->function.value() /* Function *Parent=0 */,
        0 /* BasicBlock *InsertBefore=0 */
    );
    /* Insert a branch from the current basic block to the header of the IfStmt */
    compile_scope->builder.CreateBr(IfHeaderBlock);
    /* Set the header of the IfStmt as the new insert point */
    compile_scope->builder.SetInsertPoint(IfHeaderBlock);
    llvm::Value* value_condition = this->_condition->compileRValue(compile_scope);
    // If the condition is an int32 (int1 are bools), make a check if it's not equal 0 (true) or equal 0 (false)
    if (value_condition->getType()->isIntegerTy(32)) {
        value_condition = compile_scope->builder.CreateICmpNE(value_condition, compile_scope->builder.getInt32(0));
    } else if (value_condition->getType()->isIntegerTy(8)) {
        value_condition = compile_scope->builder.CreateICmpNE(value_condition, compile_scope->builder.getInt8(0));
    } else if (value_condition->getType()->isPointerTy()) {
        // return 0 if nullptr, 1 otherwise
        value_condition = compile_scope->builder.CreatePtrToInt(value_condition, compile_scope->builder.getInt32Ty());
        value_condition = compile_scope->builder.CreateICmpNE(compile_scope->builder.getInt32(0), value_condition);
    }
    /* Change the name of the IfStmt condition (after the creation) */
    value_condition->setName("if-condition");
    /* Add a basic block for the consequence of the IfStmt */
    llvm::BasicBlock* IfConsequenceBlock = llvm::BasicBlock::Create(
        compile_scope->ctx /* LLVMContext &Context */,
        "if-consequence" /* const Twine &Name="" */,
        compile_scope->function.value() /* Function *Parent=0 */,
        0 /* BasicBlock *InsertBefore=0 */
    );
    if (this->_else_statement.has_value()) {
        /* Add a basic block for the alternative of the IfStmt */
        llvm::BasicBlock* IfAlternativeBlock = llvm::BasicBlock::Create(
            compile_scope->ctx /* LLVMContext &Context */,
            "if-alternative" /* const Twine &Name="" */,
            compile_scope->function.value() /* Function *Parent=0 */,
            0 /* BasicBlock *InsertBefore=0 */
        );
        /* Add a basic block for the end of the IfStmt (after the if) */
        llvm::BasicBlock* IfEndBlock = llvm::BasicBlock::Create(
            compile_scope->ctx /* LLVMContext &Context */,
            "if-end" /* const Twine &Name="" */,
            compile_scope->function.value() /* Function *Parent=0 */,
            0 /* BasicBlock *InsertBefore=0 */
        );
        /* Create the conditional branch */
        compile_scope->builder.CreateCondBr(value_condition, IfConsequenceBlock, IfAlternativeBlock);
        /* Set the header of the IfConsequenceBlock as the new insert point */
        compile_scope->builder.SetInsertPoint(IfConsequenceBlock);
        auto consequence_compile_scope = std::make_shared<CompileScope>(compile_scope);
        this->_then_statement->compile(consequence_compile_scope);
        /* Insert the jump to the if end block */
        compile_scope->builder.CreateBr(IfEndBlock);
        /* Set the header of the IfAlternativeBlock as the new insert point */
        compile_scope->builder.SetInsertPoint(IfAlternativeBlock);
        auto alternative_compile_scope = std::make_shared<CompileScope>(compile_scope);
        this->_else_statement.value()->compile(alternative_compile_scope);
        /* Insert the jump to the if end block */
        compile_scope->builder.CreateBr(IfEndBlock);
        /* Continue in the if end block */
        compile_scope->builder.SetInsertPoint(IfEndBlock);
    } else {
        /* Add a basic block for the end of the IfStmt (after the if) */
        llvm::BasicBlock* IfEndBlock = llvm::BasicBlock::Create(
            compile_scope->ctx /* LLVMContext &Context */,
            "if-end" /* const Twine &Name="" */,
            compile_scope->function.value() /* Function *Parent=0 */,
            0 /* BasicBlock *InsertBefore=0 */
        );
        /* Create the conditional branch */
        compile_scope->builder.CreateCondBr(value_condition, IfConsequenceBlock, IfEndBlock);
        /* Set the header of the IfConsequenceBlock as the new insert point */
        compile_scope->builder.SetInsertPoint(IfConsequenceBlock);
        auto consequence_compile_scope = std::make_shared<CompileScope>(compile_scope);
        this->_then_statement->compile(consequence_compile_scope);
        /* Insert the jump to the if end block */
        compile_scope->builder.CreateBr(IfEndBlock);
        /* Set the header of the IfAlternativeBlock as the new insert point */
        compile_scope->builder.SetInsertPoint(IfEndBlock);
    }
}

void WhileStatement::print(std::ostream& stream) {
    stream << "while (" << this->_condition << ')';
    IndentManager& indent = IndentManager::getInstance();
    if (this->_body->kind == StatementKind::ST_BLOCK) {
        stream << ' ' << this->_body;
    } else if (this->_body->kind == StatementKind::ST_LABELED) {
        indent.increaseCurrIndentation(1);
        stream << '\n' << this->_body;
        indent.decreaseCurrIndentation(1);
    } else {
        indent.increaseCurrIndentation(1);
        stream << '\n' << indent << this->_body;
        indent.decreaseCurrIndentation(1);
    }
}

void WhileStatement::typecheck(ScopePtr& scope) {
    auto condition_type = this->_condition->typecheckWrap(scope);
    if (!condition_type->isScalar()) {
        errorloc(this->loc, "Condition of a while statement must be scalar");
    }
    scope->loop_counter++;
    this->_body->typecheck(scope);
    scope->loop_counter--;
}

void WhileStatement::compile(CompileScopePtr compile_scope) {
    /* Add a basic block for the header of the WhileStmt */
    llvm::BasicBlock* WhileHeaderBlock = llvm::BasicBlock::Create(
        compile_scope->ctx /* LLVMContext &Context */,
        "while-header" /* const Twine &Name="" */,
        compile_scope->function.value() /* Function *Parent=0 */,
        0 /* BasicBlock *InsertBefore=0 */
    );
    /* Insert a branch from the current basic block to the header of the WhileStmt */
    compile_scope->builder.CreateBr(WhileHeaderBlock);

    /* Set the header of the WhileStmt as the new insert point */
    compile_scope->builder.SetInsertPoint(WhileHeaderBlock);

    llvm::Value* while_condition = this->_condition->compileRValue(compile_scope);
    // If the condition is an int32 (int1 are bools), make a check if it's not equal 0 (true) or equal 0 (false)
    if (while_condition->getType()->isIntegerTy(32)) {
        while_condition = compile_scope->builder.CreateICmpNE(while_condition, compile_scope->builder.getInt32(0));
    } else if (while_condition->getType()->isIntegerTy(8)) {
        while_condition = compile_scope->builder.CreateICmpNE(while_condition, compile_scope->builder.getInt8(0));
    } else if (while_condition->getType()->isPointerTy()) {
        // return 0 if nullptr, 1 otherwise
        while_condition = compile_scope->builder.CreatePtrToInt(while_condition, compile_scope->builder.getInt32Ty());
        while_condition = compile_scope->builder.CreateICmpNE(compile_scope->builder.getInt32(0), while_condition);
    }

    /* Add a basic block for the consequence of the WhileStmt */
    llvm::BasicBlock* WhileBodyBlock = llvm::BasicBlock::Create(
        compile_scope->ctx /* LLVMContext &Context */,
        "while-body" /* const Twine &Name="" */,
        compile_scope->function.value() /* Function *Parent=0 */,
        0 /* BasicBlock *InsertBefore=0 */
    );
    /* Add a basic block for the alternative of the IfStmt */
    llvm::BasicBlock* WhileEndBlock = llvm::BasicBlock::Create(
        compile_scope->ctx /* LLVMContext &Context */,
        "while-end" /* const Twine &Name="" */,
        compile_scope->function.value() /* Function *Parent=0 */,
        0 /* BasicBlock *InsertBefore=0 */
    );

    /* Create the conditional branch */
    compile_scope->builder.CreateCondBr(while_condition, WhileBodyBlock, WhileEndBlock);

    /* Start inserting in the while body block */
    compile_scope->builder.SetInsertPoint(WhileBodyBlock);
    auto inner_compile_scope = std::make_shared<CompileScope>(compile_scope);
    inner_compile_scope->setBreakBlock(WhileEndBlock);
    inner_compile_scope->setContinueBlock(WhileHeaderBlock);
    this->_body->compile(inner_compile_scope);
    /* Insert the back loop edge (the branch back to the header) */
    compile_scope->builder.CreateBr(WhileHeaderBlock);

    /* The while was created, adjust the inserting point to the while end block */
    compile_scope->builder.SetInsertPoint(WhileEndBlock);
}

void JumpStatement::print(std::ostream& stream) {
    stream << this->_jump_str << ';';
}

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
    std::optional<llvm::BasicBlock*> labeledBlock = compile_scope->getLabeledBlock(this->_ident);
    if (labeledBlock.has_value()) {
        compile_scope->builder.CreateBr(labeledBlock.value());
    }
    llvm::BasicBlock* ReturnDeadBlock = llvm::BasicBlock::Create(
        compile_scope->ctx /* LLVMContext &Context */,
        "DEAD_BLOCK" /* const Twine &Name="" */,
        compile_scope->function.value() /* Function *Parent=0 */,
        0 /* BasicBlock *InsertBefore=0 */
    );
    compile_scope->builder.SetInsertPoint(ReturnDeadBlock);
}

void ContinueStatement::typecheck(ScopePtr& scope) {
    if (scope->loop_counter == 0) {
        errorloc(this->loc, "Invalid 'continue' outside of a loop");
    }
}

void ContinueStatement::compile(CompileScopePtr compile_scope) {
    std::optional<llvm::BasicBlock*> ContinueBlock = compile_scope->getContinueBlock();
    if (ContinueBlock.has_value()) {
        compile_scope->builder.CreateBr(ContinueBlock.value());
    }
    llvm::BasicBlock* ReturnDeadBlock = llvm::BasicBlock::Create(
        compile_scope->ctx /* LLVMContext &Context */,
        "DEAD_BLOCK" /* const Twine &Name="" */,
        compile_scope->function.value() /* Function *Parent=0 */,
        0 /* BasicBlock *InsertBefore=0 */
    );
    compile_scope->builder.SetInsertPoint(ReturnDeadBlock);
}

void BreakStatement::typecheck(ScopePtr& scope) {
    if (scope->loop_counter == 0) {
        errorloc(this->loc, "Invalid 'break' outside of a loop");
    }
}

void BreakStatement::compile(CompileScopePtr compile_scope) {
    std::optional<llvm::BasicBlock*> BreakBlock = compile_scope->getBreakBlock();
    if (BreakBlock.has_value()) {
        compile_scope->builder.CreateBr(BreakBlock.value());
    }
    llvm::BasicBlock* ReturnDeadBlock = llvm::BasicBlock::Create(
        compile_scope->ctx /* LLVMContext &Context */,
        "DEAD_BLOCK" /* const Twine &Name="" */,
        compile_scope->function.value() /* Function *Parent=0 */,
        0 /* BasicBlock *InsertBefore=0 */
    );
    compile_scope->builder.SetInsertPoint(ReturnDeadBlock);
}

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
            return_value =
                compile_scope->builder.CreateIntCast(return_value, llvm::Type::getInt32Ty(compile_scope->ctx), false);
        }

        compile_scope->builder.CreateRet(return_value);
    } else {
        compile_scope->builder.CreateRetVoid();
    }

    /* Always create a new block after a return statement
     *
     *  This will prevent you from inserting code after a block terminator (here
     *  the return instruction), but it will create a dead basic block instead.
     */
    llvm::BasicBlock* ReturnDeadBlock = llvm::BasicBlock::Create(
        compile_scope->ctx /* LLVMContext &Context */,
        "DEAD_BLOCK" /* const Twine &Name="" */,
        compile_scope->function.value() /* Function *Parent=0 */,
        0 /* BasicBlock *InsertBefore=0 */
    );
    compile_scope->builder.SetInsertPoint(ReturnDeadBlock);
}
