#pragma once

#include <optional>
#include <unordered_map>

#include "../util/symbol_internalizer.h"

#include "llvm/IR/Module.h"                /* Module */
#include "llvm/IR/Function.h"              /* Function */
#include "llvm/IR/IRBuilder.h"             /* IRBuilder */
#include "llvm/IR/LLVMContext.h"           /* LLVMContext */
#include "llvm/IR/GlobalValue.h"           /* GlobaleVariable, LinkageTypes */
#include "llvm/IR/Verifier.h"              /* verifyFunction, verifyModule */
#include "llvm/Support/Signals.h"          /* Nice stacktrace output */
#include "llvm/Support/SystemUtils.h"
#include "llvm/Support/PrettyStackTrace.h"

/*
    This CompileScope saves Allocas of variables so that we can create Saves and Stores with their value pointers
    Also saves important llvm classes for easy access (don't have to pass them in function params)
*/
struct CompileScope {
    // Constructor for first CompileScope
    CompileScope(llvm::IRBuilder<>& Builder, llvm::IRBuilder<>& AllocaBuilder, llvm::Module& Module, llvm::LLVMContext& Ctx) :
    _Parent(std::nullopt), _Builder(Builder), _AllocaBuilder(AllocaBuilder), _Module(Module), _Ctx(Ctx) {};
    // Construct for CompileScopes with Parent and ParentFunction
    CompileScope(std::shared_ptr<CompileScope> Parent, llvm::Function* ParentFunction) :
    _Parent(Parent), _Builder(Parent->_Builder), _AllocaBuilder(Parent->_AllocaBuilder), _Module(Parent->_Module), _Ctx(Parent->_Ctx), 
    _ParentFunction(ParentFunction) {};
    // Construct for CompileScopes with Parent
    CompileScope(std::shared_ptr<CompileScope> Parent) :
    _Parent(Parent), _Builder(Parent->_Builder), _AllocaBuilder(Parent->_AllocaBuilder), _Module(Parent->_Module), _Ctx(Parent->_Ctx), 
    _ParentFunction(Parent->_ParentFunction) {};
    
    std::optional<llvm::Value*> getAlloca(Symbol var) {
        if (this->_Allocas.find(var) == this->_Allocas.end()) {
            if (!this->_Parent.has_value()) {
                if (_Module.getGlobalVariable(*var) == NULL) {
                    return std::nullopt;
                }
                return _Module.getGlobalVariable(*var);
            }
            return this->_Parent.value()->getAlloca(var);
        }
        return this->_Allocas.at(var);
    }

    void addAlloca(Symbol var, llvm::Value* allocaa) {
        this->_Allocas.insert({var, allocaa});
    }

    std::optional<llvm::Type*> getType(Symbol var) {
        if (this->_Types.find(var) == this->_Types.end()) {
            if (!this->_Parent.has_value()) {
                return std::nullopt;
            }
            return this->_Parent.value()->getType(var);
        }
        return this->_Types.at(var);
    }

    void addType(Symbol var, llvm::Type* type) {
        this->_Types.insert({var, type});
    }

    std::optional<std::shared_ptr<CompileScope>> _Parent;
    llvm::IRBuilder<>& _Builder;
    llvm::IRBuilder<>& _AllocaBuilder;
    llvm::Module& _Module;
    llvm::LLVMContext& _Ctx;
    std::optional<llvm::Function*> _ParentFunction;
    std::unordered_map<Symbol, llvm::Value*> _Allocas;
    std::unordered_map<Symbol, llvm::Type*> _Types;
};

// std::shared_ptr<CompileScope> CompileScopePtr;