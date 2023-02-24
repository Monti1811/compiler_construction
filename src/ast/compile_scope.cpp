#include "compile_scope.h"

// Constructor for first CompileScope
CompileScope::CompileScope(
    llvm::IRBuilder<>& builder, llvm::IRBuilder<>& alloca_builder, llvm::Module& module, llvm::LLVMContext& ctx
)
    : builder(builder)
    , alloca_builder(alloca_builder)
    , module(module)
    , ctx(ctx){};

// Constructor for a CompileScope in a function
CompileScope::CompileScope(CompileScopePtr _parent, llvm::Function* function)
    : builder(_parent->builder)
    , alloca_builder(_parent->alloca_builder)
    , module(_parent->module)
    , ctx(_parent->ctx)
    , function(function)
    , _parent(_parent)
    , _break_block(_parent->getBreakBlock())
    , _continue_block(_parent->getContinueBlock()){};

// Constructor for CompileScopes with _parent
CompileScope::CompileScope(CompileScopePtr _parent)
    : builder(_parent->builder)
    , alloca_builder(_parent->alloca_builder)
    , module(_parent->module)
    , ctx(_parent->ctx)
    , function(_parent->function)
    , _parent(_parent)
    , _break_block(_parent->getBreakBlock())
    , _continue_block(_parent->getContinueBlock()){};

std::optional<llvm::Value*> CompileScope::getAlloca(Symbol var) {
    if (this->_allocas.find(var) == this->_allocas.end()) {
        if (!this->_parent.has_value()) {
            auto var_alloc = this->module.getGlobalVariable(*var);
            if (var_alloc != NULL) {
                return var_alloc;
            }
            auto function_alloc = this->module.getFunction(*var);
            if (function_alloc != NULL) {
                return function_alloc;
            }
            return std::nullopt;
        }
        return this->_parent.value()->getAlloca(var);
    }
    return this->_allocas.at(var);
}

void CompileScope::addAlloca(Symbol var, llvm::Value* allocaa) {
    this->_allocas.insert({var, allocaa});
}

std::optional<llvm::Type*> CompileScope::getType(Symbol var) {
    if (this->_types.find(var) != this->_types.end()) {
        return this->_types.at(var);
    }

    if (this->_parent.has_value()) {
        return this->_parent.value()->getType(var);
    }

    auto var_alloc = this->module.getGlobalVariable(*var);
    if (var_alloc != NULL) {
        return var_alloc->getType();
    }
    auto function_alloc = this->module.getFunction(*var);
    if (function_alloc != NULL) {
        return function_alloc->getFunctionType();
    }
    return std::nullopt;
}

void CompileScope::addType(Symbol var, llvm::Type* type) {
    this->_types.insert({var, type});
}

void CompileScope::addLabeledBlock(Symbol name, llvm::BasicBlock* LabeledBlock) {
    this->_labeled_blocks.insert({name, LabeledBlock});
}

std::optional<llvm::BasicBlock*> CompileScope::getLabeledBlock(Symbol name) {
    if (this->_labeled_blocks.find(name) == this->_labeled_blocks.end()) {
        if (!this->_parent.has_value()) {
            return std::nullopt;
        }
        return this->_parent.value()->getLabeledBlock(name);
    }
    return this->_labeled_blocks.at(name);
}

void CompileScope::setBreakBlock(llvm::BasicBlock* BreakBlock) {
    this->_break_block = BreakBlock;
}
std::optional<llvm::BasicBlock*> CompileScope::getBreakBlock() {
    return this->_break_block;
}
void CompileScope::setContinueBlock(llvm::BasicBlock* ContinueBlock) {
    this->_continue_block = ContinueBlock;
}
std::optional<llvm::BasicBlock*> CompileScope::getContinueBlock() {
    return this->_continue_block;
}
