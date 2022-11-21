#include <vector>

#include "declarator.h"
#include "expression.h"
#include "statement.h"

struct ExternalDeclaration {};

struct FunctionDefinition: public ExternalDeclaration {
    Declaration declaration;
    BlockStatement block;
};

// TODO: This is kinda ugly
struct WrappedDeclaration: public ExternalDeclaration, public BlockItem {
    Declaration declaration;
};
