#pragma once

#include <memory>
#include <unordered_set>
#include <vector>

#include "compile_scope.h"
#include "declaration.h"
#include "expressions.h"
#include "function_definition.h"
#include "statement.h"

struct Program {
  public:
    Program(){};

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
