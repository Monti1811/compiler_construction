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
    CompileScope(
        llvm::IRBuilder<>& builder, llvm::IRBuilder<>& alloca_builder, llvm::Module& module, llvm::LLVMContext& ctx
    );

    // Constructor for a CompileScope in a function
    CompileScope(std::shared_ptr<CompileScope> parent, llvm::Function* function);

    // Constructor for CompileScopes with _parent
    CompileScope(std::shared_ptr<CompileScope> parent);

    std::optional<llvm::Value*> getAlloca(Symbol var);

    void addAlloca(Symbol var, llvm::Value* alloca);

    std::optional<llvm::Type*> getType(Symbol var);

    void addType(Symbol var, llvm::Type* type);

    void addLabeledBlock(Symbol name, llvm::BasicBlock* LabeledBlock);

    std::optional<llvm::BasicBlock*> getLabeledBlock(Symbol name);

    void setBreakBlock(llvm::BasicBlock* BreakBlock);

    std::optional<llvm::BasicBlock*> getBreakBlock();

    void setContinueBlock(llvm::BasicBlock* ContinueBlock);

    std::optional<llvm::BasicBlock*> getContinueBlock();

    void addFunctionPointer(std::string var, std::string function);

    std::optional<llvm::Function*> getFunctionPointer(std::string var);

    llvm::IRBuilder<>& builder;
    llvm::IRBuilder<>& alloca_builder;
    llvm::Module& module;
    llvm::LLVMContext& ctx;
    std::optional<llvm::Function*> function;

  private:
    std::optional<std::shared_ptr<CompileScope>> _parent;

    std::unordered_map<Symbol, llvm::Value*> _allocas;
    std::unordered_map<Symbol, llvm::Type*> _types;
    std::unordered_map<Symbol, llvm::BasicBlock*> _labeled_blocks;
    std::unordered_map<std::string, std::string> _function_pointers;
    std::optional<llvm::BasicBlock*> _break_block;
    std::optional<llvm::BasicBlock*> _continue_block;
};

typedef std::shared_ptr<CompileScope> CompileScopePtr;
