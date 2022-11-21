#include <vector>

#include "declarator.h"
#include "expression.h"
#include "statement.h"

struct ExternalDeclaration {};

struct FunctionDefinition: public ExternalDeclaration {
    Declaration declaration;
    BlockStatement block;
};

// TODO: Duplicated in `declarator.h`
struct Declaration: public ExternalDeclaration, public BlockItem {
    std::vector<TypeSpecifier> specifiers;
    Declarator declarator;
};
