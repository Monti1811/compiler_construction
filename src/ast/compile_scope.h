#pragma once

#include <optional>
#include <unordered_map>

#include "../util/symbol_internalizer.h"

#include "../llvm.h"

/*
    This CompileScope saves Allocas of variables so that we can create Saves and Stores with their value pointers
    Also saves important llvm classes for easy access (don't have to pass them in function params)
*/
struct CompileScope {
    // Constructor for first CompileScope
    CompileScope(llvm::IRBuilder<>& Builder, llvm::IRBuilder<>& AllocaBuilder, llvm::Module& Module, llvm::LLVMContext& Ctx);
    // Construct for CompileScopes with Parent and ParentFunction
    CompileScope(std::shared_ptr<CompileScope> Parent, llvm::Function* ParentFunction);
    // Construct for CompileScopes with Parent
    CompileScope(std::shared_ptr<CompileScope> Parent);
    
    std::optional<llvm::Value*> getAlloca(Symbol var);

    void addAlloca(Symbol var, llvm::Value* alloca);

    std::optional<llvm::Type*> getType(Symbol var);

    void addType(Symbol var, llvm::Type* type);

    void addLabeledBlock(Symbol name, llvm::BasicBlock* LabeledBlock);

    std::optional<llvm::BasicBlock*> getLabeledBlock(Symbol name);

    void setBreakBlock(llvm::BasicBlock *BreakBlock);

    std::optional<llvm::BasicBlock*> getBreakBlock();

    void setContinueBlock(llvm::BasicBlock *ContinueBlock);

    std::optional<llvm::BasicBlock*> getContinueBlock();

    void addFunctionPointer(std::string var, std::string function);

    std::optional<llvm::Function*> getFunctionPointer(std::string var);

    std::optional<std::shared_ptr<CompileScope>> _Parent;
    llvm::IRBuilder<>& _Builder;
    llvm::IRBuilder<>& _AllocaBuilder;
    llvm::Module& _Module;
    llvm::LLVMContext& _Ctx;
    std::optional<llvm::Function*> _ParentFunction;
    private:
    std::unordered_map<Symbol, llvm::Value*> _Allocas;
    std::unordered_map<Symbol, llvm::Type*> _Types;
    std::unordered_map<Symbol, llvm::BasicBlock*> _LabeledBlocks;
    std::unordered_map<std::string, std::string> _FunctionPointers;
    std::optional<llvm::BasicBlock*> _BreakBlock;
    std::optional<llvm::BasicBlock*> _ContinueBlock;
};
