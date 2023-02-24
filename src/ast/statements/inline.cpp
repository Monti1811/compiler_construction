#include "inline.h"

#include "../indentation.h"

// LabeledStatement

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
    // Retrieve the block with this label
    auto labeled_block_opt = compile_scope->getLabeledBlock(this->_name);
    if (!labeled_block_opt.has_value()) {
        errorloc(this->loc, "Unknown label ", *this->_name);
    }
    auto labeled_block = labeled_block_opt.value();

    // Create a jump to the label
    compile_scope->builder.CreateBr(labeled_block);
    compile_scope->builder.SetInsertPoint(labeled_block);

    // Compile the inner statement
    auto inner_compile_scope = std::make_shared<CompileScope>(compile_scope);
    this->_inner->compile(inner_compile_scope);
}

// DeclarationStatement

void DeclarationStatement::print(std::ostream& stream) {
    stream << this->_declaration << ';';
}

void DeclarationStatement::typecheck(ScopePtr& scope) {
    this->_declaration.typecheck(scope);
}

void DeclarationStatement::compile(CompileScopePtr compile_scope) {
    // Retrieve type and name of the declaration
    TypePtr type_dcl = this->_declaration.getTypeDecl().type;
    auto name = this->_declaration.getTypeDecl().name;

    // Do nothing if it is abstract
    if (!name.has_value()) {
        return;
    }

    llvm::Type* llvm_type = type_dcl->toLLVMType(compile_scope);

    // Allocate stack space for the variable
    llvm::Value* LocalVarPtr = compile_scope->alloca_builder.CreateAlloca(llvm_type);

    // Reset the alloca builder each time before using it
    compile_scope->alloca_builder.SetInsertPoint(
        compile_scope->alloca_builder.GetInsertBlock(), compile_scope->alloca_builder.GetInsertBlock()->begin()
    );

    // Fill the compile scope
    compile_scope->addAlloca(name.value(), LocalVarPtr);
    compile_scope->addType(name.value(), llvm_type);
}

// ExpressionStatement

void ExpressionStatement::print(std::ostream& stream) {
    stream << this->_expr << ';';
}

void ExpressionStatement::typecheck(ScopePtr& scope) {
    this->_expr->typecheck(scope);
}

void ExpressionStatement::compile(CompileScopePtr compile_scope) {
    this->_expr->compileRValue(compile_scope);
}
