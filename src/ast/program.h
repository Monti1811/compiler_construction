#pragma once

#include <vector>

#include "declarator.h"
#include "expression.h"
#include "statement.h"

struct FunctionDefinition {
    Declaration declaration;
    BlockStatement block;
};

struct Program {
    public:
    Program() {};

    void print(std::ostream& stream);
    friend std::ostream& operator<<(std::ostream& stream, Program& program);

    void addDeclaration(Declaration declaration);
    void addFunctionDefinition(FunctionDefinition definition);

    private:
    std::vector<Declaration> _declarations;
    std::vector<FunctionDefinition> _functions;
    std::vector<bool> _is_declaration;
};
