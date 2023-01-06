#pragma once

#include <memory>
#include <unordered_set>
#include <vector>

#include "declarator.h"
#include "expression.h"
#include "statement.h"

struct FunctionDefinition {
    public:
    FunctionDefinition(Declaration declaration, std::optional<BlockStatement> block, std::unordered_set<Symbol> labels)
        : _declaration(std::move(declaration))
        , _block(std::move(block))
        , _labels(labels) {};

    void print(std::ostream& stream);
    friend std::ostream& operator<<(std::ostream& stream, FunctionDefinition& definition);

    void typecheck(ScopePtr& scope);
    bool isAbstract() {
        return !_block.has_value();
    }
    void checkDefinition(ScopePtr& scope);


    private:
    Declaration _declaration;
    std::optional<BlockStatement> _block;
    std::unordered_set<Symbol> _labels;
};

struct Program {
    public:
    Program() {};

    void print(std::ostream& stream);
    friend std::ostream& operator<<(std::ostream& stream, Program& program);

    void addDeclaration(Declaration declaration);
    void addFunctionDefinition(FunctionDefinition definition);
    void typecheck();

    private:
    std::vector<Declaration> _declarations;
    std::vector<FunctionDefinition> _functions;
    std::vector<bool> _is_declaration;
};
