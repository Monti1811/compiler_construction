#pragma once

#include <memory>

#include "types/type.h"

struct Scope {
    Scope() : parent(std::nullopt) {};
    Scope(std::shared_ptr<Scope> parent)
        : parent(parent)
        , function_return_type(parent->function_return_type)
        , loop_counter(parent->loop_counter) {};

    std::optional<TypePtr> getVarType(Symbol ident);

    std::optional<std::shared_ptr<StructType>> getStructType(Symbol ident);

    bool isLabelDefined(Symbol label);
    void setLabels(std::unordered_set<Symbol> labels);

    // Returns whether the variable was already defined
    bool addDeclaration(TypeDecl& decl);

    // Returns whether the function was already defined
    bool addFunctionDeclaration(TypeDecl& decl);

    bool isStructDefined(Symbol& name);

    // Returns whether the struct was already defined
    bool addStruct(std::shared_ptr<StructType> type);

    std::optional<std::shared_ptr<Scope>> parent;

    std::unordered_map<Symbol, TypePtr> vars;
    std::unordered_set<Symbol> defined_functions;

    std::unordered_map<Symbol, std::shared_ptr<StructType>> structs;

    std::unordered_set<Symbol> labels;
    
    // used to typecheck a return-statement
    std::optional<TypePtr> function_return_type;

    int loop_counter = 0;
};

typedef std::shared_ptr<Scope> ScopePtr;
