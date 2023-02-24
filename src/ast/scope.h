#pragma once

#include <memory>

#include "types/struct.h"
#include "types/type.h"

struct Scope {
  public:
    Scope()
        : _parent(std::nullopt)
        , _root(true){};

    Scope(std::shared_ptr<Scope> parent)
        : function_return_type(parent->function_return_type)
        , loop_counter(parent->loop_counter)
        , scope_counter(parent->scope_counter + 1)
        , _parent(parent)
        , _root(false){};

    std::optional<TypePtr> getVarType(Symbol ident);

    std::optional<std::shared_ptr<StructType>> getStructType(Symbol ident);

    std::optional<std::shared_ptr<CompleteStructType>> getCompleteStruct(StructType struct_type);

    bool isLabelDefined(Symbol label);
    void setLabels(std::unordered_set<Symbol> labels);

    // Returns whether the variable was already defined
    bool addDeclaration(TypeDecl& decl, bool function_param = false);

    // Returns whether the function was already defined
    bool addFunctionDeclaration(TypeDecl& decl);

    // Returns whether a variable is a function designator
    bool isFunctionDesignator(Symbol& name);

    bool isStructDefined(Symbol& name);

    // Returns whether the struct was already defined
    bool addStruct(std::shared_ptr<StructType> type);

    // Used to typecheck a return statement
    std::optional<TypePtr> function_return_type;

    int loop_counter = 0;
    int scope_counter = 0;

  private:
    std::optional<std::shared_ptr<Scope>> _parent;
    bool _root;

    std::unordered_map<Symbol, TypePtr> _vars;
    std::unordered_set<Symbol> _defined_functions;
    std::unordered_map<Symbol, std::shared_ptr<StructType>> _structs;
    std::unordered_set<Symbol> _labels;
};

typedef std::shared_ptr<Scope> ScopePtr;
