#pragma once

#include <memory>
#include <unordered_set>
#include <vector>

#include "declaration.h"
#include "expression.h"
#include "statement.h"

struct FunctionDefinition {
    public:
    FunctionDefinition(Declaration declaration, BlockStatement block, std::unordered_set<Symbol> labels)
        : _declaration(std::move(declaration))
        , _block(std::move(block))
        , _labels(labels) {};

    void print(std::ostream& stream);
    friend std::ostream& operator<<(std::ostream& stream, FunctionDefinition& definition);

    void typecheck(ScopePtr& scope);
    FunctionType getFunctionType() {
        return type.value();
    }

    Declaration _declaration;
    private:
    BlockStatement _block;
    std::unordered_set<Symbol> _labels;
    // TODO: add this while typechecking
    std::optional<FunctionType> type;
};

struct Program {
    public:
    Program() {};

    void print(std::ostream& stream);
    friend std::ostream& operator<<(std::ostream& stream, Program& program);

    void addDeclaration(Declaration declaration);
    void addFunctionDefinition(FunctionDefinition definition);
    void typecheck();
    void compile(std::string filename);

    private:
    std::vector<Declaration> _declarations;
    std::vector<FunctionDefinition> _functions;
    std::vector<bool> _is_declaration;
};
