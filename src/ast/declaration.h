#include <vector>

#include "declarator.h"
#include "expression.h"
#include "statement.h"
#include "specifier.h"

struct ExternalDeclaration {};

struct FunctionDefinition: public ExternalDeclaration {
    std::vector<Specifier> specifiers;
    Declarator declarator;
    CompoundStatement compound_statement;
};

struct Declaration1: public ExternalDeclaration {
    std::vector<Specifier> specifiers;
    Declarator declarator;
};

struct Declaration2: public ExternalDeclaration {
    std::vector<Specifier> specifiers;
    std::vector<Initializer> initializers;
};

struct InitDeclarator {};

struct BaseInitDeclarator: public InitDeclarator {
    Declarator declarator;
};

struct InitializedDeclarator: public InitDeclarator {
    // declarator = initializer
    Declarator declarator;
    Initializer initializer;
}

struct Initializer {};

struct AssignmentInitializer: public Initializer {
    AssignmentExpression inner;
};

struct InitializerList: public Initializer {
    std::vector<Initializer> initializers;
}
