#include "controlflow.h"

#include "../indentation.h"

// IfStatement

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

// WhileStatement

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
