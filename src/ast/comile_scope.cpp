#include "compile_scope.h"

// Constructor for first CompileScope
CompileScope::CompileScope(
    llvm::IRBuilder<>& Builder, llvm::IRBuilder<>& AllocaBuilder, llvm::Module& Module, llvm::LLVMContext& Ctx
)
    : _Parent(std::nullopt)
    , _Builder(Builder)
    , _AllocaBuilder(AllocaBuilder)
    , _Module(Module)
    , _Ctx(Ctx){};
// Construct for CompileScopes with Parent and ParentFunction
CompileScope::CompileScope(std::shared_ptr<CompileScope> Parent, llvm::Function* ParentFunction)
    : _Parent(Parent)
    , _Builder(Parent->_Builder)
    , _AllocaBuilder(Parent->_AllocaBuilder)
    , _Module(Parent->_Module)
    , _Ctx(Parent->_Ctx)
    , _ParentFunction(ParentFunction)
    , _BreakBlock(Parent->getBreakBlock())
    , _ContinueBlock(Parent->getContinueBlock()){};
// Construct for CompileScopes with Parent
CompileScope::CompileScope(std::shared_ptr<CompileScope> Parent)
    : _Parent(Parent)
    , _Builder(Parent->_Builder)
    , _AllocaBuilder(Parent->_AllocaBuilder)
    , _Module(Parent->_Module)
    , _Ctx(Parent->_Ctx)
    , _ParentFunction(Parent->_ParentFunction)
    , _BreakBlock(Parent->getBreakBlock())
    , _ContinueBlock(Parent->getContinueBlock()){};

std::optional<llvm::Value*> CompileScope::getAlloca(Symbol var) {
    if (this->_Allocas.find(var) == this->_Allocas.end()) {
        if (!this->_Parent.has_value()) {
            auto var_alloc = _Module.getGlobalVariable(*var);
            if (var_alloc != NULL) {
                return var_alloc;
            }
            auto function_alloc = _Module.getFunction(*var);
            if (function_alloc != NULL) {
                return function_alloc;
            }
            return std::nullopt;
        }
        return this->_Parent.value()->getAlloca(var);
    }
    return this->_Allocas.at(var);
}

void CompileScope::addAlloca(Symbol var, llvm::Value* allocaa) {
    this->_Allocas.insert({var, allocaa});
}

std::optional<llvm::Type*> CompileScope::getType(Symbol var) {
    if (this->_Types.find(var) != this->_Types.end()) {
        return this->_Types.at(var);
    }

    if (this->_Parent.has_value()) {
        return this->_Parent.value()->getType(var);
    }

    auto var_alloc = _Module.getGlobalVariable(*var);
    if (var_alloc != NULL) {
        return var_alloc->getType();
    }
    auto function_alloc = _Module.getFunction(*var);
    if (function_alloc != NULL) {
        return function_alloc->getFunctionType();
    }
    return std::nullopt;
}

void CompileScope::addType(Symbol var, llvm::Type* type) {
    this->_Types.insert({var, type});
}

void CompileScope::addLabeledBlock(Symbol name, llvm::BasicBlock* LabeledBlock) {
    this->_LabeledBlocks.insert({name, LabeledBlock});
}

std::optional<llvm::BasicBlock*> CompileScope::getLabeledBlock(Symbol name) {
    if (this->_LabeledBlocks.find(name) == this->_LabeledBlocks.end()) {
        if (!this->_Parent.has_value()) {
            return std::nullopt;
        }
        return this->_Parent.value()->getLabeledBlock(name);
    }
    return this->_LabeledBlocks.at(name);
}

void CompileScope::setBreakBlock(llvm::BasicBlock* BreakBlock) {
    this->_BreakBlock = BreakBlock;
}
std::optional<llvm::BasicBlock*> CompileScope::getBreakBlock() {
    return this->_BreakBlock;
}
void CompileScope::setContinueBlock(llvm::BasicBlock* ContinueBlock) {
    this->_ContinueBlock = ContinueBlock;
}
std::optional<llvm::BasicBlock*> CompileScope::getContinueBlock() {
    return this->_ContinueBlock;
}

void CompileScope::addFunctionPointer(std::string var, std::string function) {
    this->_FunctionPointers.insert({var, function});
}

std::optional<llvm::Function*> CompileScope::getFunctionPointer(std::string var) {
    if (this->_FunctionPointers.find(var) == this->_FunctionPointers.end()) {
        if (!this->_Parent.has_value()) {
            auto function_alloc = _Module.getFunction(var);
            if (function_alloc != NULL) {
                return function_alloc;
            }
            return std::nullopt;
        }
        return this->_Parent.value()->getFunctionPointer(var);
    }
    return this->getFunctionPointer(this->_FunctionPointers.at(var));
}