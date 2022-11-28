#include <vector>

#include "declarator.h"
#include "expression.h"
#include "statement.h"

struct FunctionDefinition {
    Declaration declaration;
    BlockStatement block;
};

struct Program {
    std::vector<Declaration> decls;
    std::vector<FunctionDefinition> funcs;

    // TODO Preserve order when printing
};
