#pragma once

#include "declarators/declarator.h"
#include "specifiers/specifier.h"

#include "type_decl.h"

#include "llvm/IR/Module.h"                /* Module */
#include "llvm/IR/Function.h"              /* Function */
#include "llvm/IR/IRBuilder.h"             /* IRBuilder */
#include "llvm/IR/LLVMContext.h"           /* LLVMContext */
#include "llvm/IR/GlobalValue.h"           /* GlobaleVariable, LinkageTypes */
#include "llvm/IR/Verifier.h"              /* verifyFunction, verifyModule */
#include "llvm/Support/Signals.h"          /* Nice stacktrace output */
#include "llvm/Support/SystemUtils.h"
#include "llvm/Support/PrettyStackTrace.h"

// int x;
// ^   ^
// |   declarator
// type-specifier
struct Declaration {
    Declaration(Locatable loc, TypeSpecifierPtr specifier, DeclaratorPtr declarator)
        : _loc(loc)
        , _specifier(std::move(specifier))
        , _declarator(std::move(declarator)) {};

    void print(std::ostream& stream);
    friend std::ostream& operator<<(std::ostream& stream, Declaration& declaration);

    void typecheck(ScopePtr& scope);
    TypeDecl toType(ScopePtr& scope);
    TypeDecl getTypeDecl();

    Locatable _loc;
    TypeSpecifierPtr _specifier;
    DeclaratorPtr _declarator;
    private:
    // TODO: Add this while typechecking
    std::optional<TypeDecl> _typeDecl;
};
