#include <vector>

#include "declarator.h"
#include "expression.h"
#include "statement.h"

struct ExternalDeclaration {};

struct FunctionDefinition: public ExternalDeclaration {
    std::vector<Specifier> specifiers;
    Declarator declarator;
    CompoundStatement compound_statement;
};

struct Declaration: public ExternalDeclaration {
    std::vector<Specifier> specifiers;
    Declarator declarator;
};
