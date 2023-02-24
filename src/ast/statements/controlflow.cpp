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
    // Add a basic block for the header of the if statement
    llvm::BasicBlock* if_header_block = llvm::BasicBlock::Create(
        compile_scope->ctx,
        "if-header",
        compile_scope->function.value()
    );

    // Insert a branch from the current basic block to the header of the if statement
    compile_scope->builder.CreateBr(if_header_block);

    // Set the header of the if statement as the new insert point
    compile_scope->builder.SetInsertPoint(if_header_block);

    // Compile the condition:
    llvm::Value* value_condition = this->_condition->compileRValue(compile_scope);

    // Check whether the condition is equal or not equal to 0
    if (value_condition->getType()->isIntegerTy(32)) {
        value_condition = compile_scope->builder.CreateICmpNE(value_condition, compile_scope->builder.getInt32(0));
    } else if (value_condition->getType()->isIntegerTy(8)) {
        value_condition = compile_scope->builder.CreateICmpNE(value_condition, compile_scope->builder.getInt8(0));
    } else if (value_condition->getType()->isPointerTy()) {
        // Convert pointer to int for the comparison
        value_condition = compile_scope->builder.CreatePtrToInt(value_condition, compile_scope->builder.getInt32Ty());
        value_condition = compile_scope->builder.CreateICmpNE(compile_scope->builder.getInt32(0), value_condition);
    }

    // Change the name of the if statement condition (after its creation)
    value_condition->setName("if-condition");

    // Add a basic block for the then case of the if statement
    llvm::BasicBlock* then_block = llvm::BasicBlock::Create(
        compile_scope->ctx,
        "if-consequence",
        compile_scope->function.value()
    );

    if (this->_else_statement.has_value()) {
        // Code generation for if-else statement

        // Add a basic block for the else case of the if statement
        llvm::BasicBlock* else_block = llvm::BasicBlock::Create(
            compile_scope->ctx,
            "if-alternative",
            compile_scope->function.value()
        );

        // Add a basic block for the end of the IfStatement
        llvm::BasicBlock* end_block = llvm::BasicBlock::Create(
            compile_scope->ctx,
            "if-end",
            compile_scope->function.value()
        );

        // Create the conditional branch
        compile_scope->builder.CreateCondBr(value_condition, then_block, else_block);

        // Compile the then statement
        compile_scope->builder.SetInsertPoint(then_block);
        auto then_scope = std::make_shared<CompileScope>(compile_scope);
        this->_then_statement->compile(then_scope);

        // Insert the jump to the end block
        compile_scope->builder.CreateBr(end_block);

        // Compile the else statement
        compile_scope->builder.SetInsertPoint(else_block);
        auto else_scope = std::make_shared<CompileScope>(compile_scope);
        this->_else_statement.value()->compile(else_scope);

        // Insert the jump to the end block
        compile_scope->builder.CreateBr(end_block);

        // Continue in the if end block
        compile_scope->builder.SetInsertPoint(end_block);
    } else {
        // Code generation for if statement without else case

        // Add a basic block for the end of the IfStatement
        llvm::BasicBlock* end_block = llvm::BasicBlock::Create(
            compile_scope->ctx,
            "if-end",
            compile_scope->function.value()
        );

        // Create the conditional branch
        compile_scope->builder.CreateCondBr(value_condition, then_block, end_block);

        // Compile the then statement
        compile_scope->builder.SetInsertPoint(then_block);
        auto then_scope = std::make_shared<CompileScope>(compile_scope);
        this->_then_statement->compile(then_scope);

        // Insert the jump to the end block
        compile_scope->builder.CreateBr(end_block);

        // Continue in the if end block
        compile_scope->builder.SetInsertPoint(end_block);
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
    // Add a basic block for the header of the while statement
    llvm::BasicBlock* while_header_block = llvm::BasicBlock::Create(
        compile_scope->ctx,
        "while-header",
        compile_scope->function.value()
    );

    // Insert a branch from the current basic block to the header of the while statement
    compile_scope->builder.CreateBr(while_header_block);

    // Set the header of the while statement as the new insert point
    compile_scope->builder.SetInsertPoint(while_header_block);

    // Compile the condition:
    llvm::Value* while_condition = this->_condition->compileRValue(compile_scope);

    // Check whether the condition is equal or not equal to 0
    if (while_condition->getType()->isIntegerTy(32)) {
        while_condition = compile_scope->builder.CreateICmpNE(while_condition, compile_scope->builder.getInt32(0));
    } else if (while_condition->getType()->isIntegerTy(8)) {
        while_condition = compile_scope->builder.CreateICmpNE(while_condition, compile_scope->builder.getInt8(0));
    } else if (while_condition->getType()->isPointerTy()) {
        // Convert pointer to int for the comparison
        while_condition = compile_scope->builder.CreatePtrToInt(while_condition, compile_scope->builder.getInt32Ty());
        while_condition = compile_scope->builder.CreateICmpNE(compile_scope->builder.getInt32(0), while_condition);
    }

    // Add a basic block for the loop statement
    llvm::BasicBlock* loop_block = llvm::BasicBlock::Create(
        compile_scope->ctx,
        "while-body",
        compile_scope->function.value()
    );

    // Add a basic block for the end of the WhileStatement
    llvm::BasicBlock* end_block = llvm::BasicBlock::Create(
        compile_scope->ctx,
        "while-end",
        compile_scope->function.value()
    );

    // Create the conditional branch
    compile_scope->builder.CreateCondBr(while_condition, loop_block, end_block);

    // Compile the loop statement
    compile_scope->builder.SetInsertPoint(loop_block);
    auto loop_scope = std::make_shared<CompileScope>(compile_scope);
    loop_scope->setBreakBlock(end_block);
    loop_scope->setContinueBlock(while_header_block);
    this->_body->compile(loop_scope);

    // Insert the back loop edge (the branch back to the header)
    compile_scope->builder.CreateBr(while_header_block);

    // The while was created, adjust the inserting point to the while end block
    compile_scope->builder.SetInsertPoint(end_block);
}
