#pragma once

#include <memory>
#include <vector>

#include "declarator.h"
#include "expression.h"
#include "statement.h"

struct FunctionDefinition {
    public:
    FunctionDefinition(Declaration declaration, BlockStatement block)
        : _declaration(std::move(declaration))
        , _block(std::move(block)) {};

    void print(std::ostream& stream);
    friend std::ostream& operator<<(std::ostream& stream, FunctionDefinition& definition);

    void typecheck(ScopePtr& scope);

    private:
    Declaration _declaration;
    BlockStatement _block;
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
