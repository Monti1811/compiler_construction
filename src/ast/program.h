#pragma once

#include <memory>
#include <unordered_set>
#include <vector>

#include "declaration.h"
#include "expression.h"
#include "statement.h"
#include "compile_scope.h"

struct FunctionDefinition {
    public:
    FunctionDefinition(Declaration declaration, BlockStatement block, std::unordered_set<Symbol> labels)
        : _declaration(std::move(declaration))
        , _block(std::move(block))
        , _labels(labels) {};

    void print(std::ostream& stream);
    friend std::ostream& operator<<(std::ostream& stream, FunctionDefinition& definition);

    void typecheck(ScopePtr& scope);
    std::shared_ptr<FunctionType> getFunctionType() {
        return type.value();
    }

    BlockStatement _block;
    Declaration _declaration;
    std::unordered_set<Symbol> _labels;
    private:
    std::optional<std::shared_ptr<FunctionType>> type;
};

struct Program {
    public:
    Program() {};

    void print(std::ostream& stream);
    friend std::ostream& operator<<(std::ostream& stream, Program& program);

    void addDeclaration(Declaration declaration);
    void addFunctionDefinition(FunctionDefinition definition);
    void typecheck();
    void compile(int argc, char const* argv[], std::string filename);

    private:
    std::vector<Declaration> _declarations;
    std::vector<FunctionDefinition> _functions;
    std::vector<bool> _is_declaration;
};
