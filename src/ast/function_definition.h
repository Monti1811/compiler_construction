#pragma once

#include <memory>
#include <unordered_set>
#include <vector>

#include "compile_scope.h"
#include "declaration.h"
#include "expressions.h"
#include "statements.h"

struct FunctionDefinition {
  public:
    FunctionDefinition(Declaration declaration, BlockStatement block, std::unordered_set<Symbol> labels)
        : _declaration(std::move(declaration))
        , _block(std::move(block))
        , _labels(labels){};

    void print(std::ostream& stream);
    friend std::ostream& operator<<(std::ostream& stream, FunctionDefinition& definition);

    void typecheck(ScopePtr& scope);

    void compile(CompileScopePtr compile_scope);

  private:
    Declaration _declaration;
    BlockStatement _block;
    std::unordered_set<Symbol> _labels;
    std::shared_ptr<FunctionType> _type;
};
