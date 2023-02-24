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

// DeclarationStatement

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
