#include <string>
#include <vector>

#include "specifier.h"
#include "../util/diagnostic.h"

struct SpecDecl {
    SpecDecl(Specifier spec, Declarator decl) : specifier(spec), declarator(decl) {};

    Specifier specifier;
    Declarator declarator;
};

struct Declarator {


    bool isEmptyDeclarator();
};

struct PrimitiveDeclarator : public Declarator {
    // Identifier
    std::string name;
};

struct AbstractDeclarator : public Declarator {};

struct PointerDeclarator : public AbstractDeclarator {
    // Pointer
    Declarator inner;
};

struct DirectAbstractDeclarator : public AbstractDeclarator {};

struct DirectAbstractDeclaratorStar : public DirectAbstractDeclarator {
    DirectAbstractDeclarator inner;
    ParameterList parameters;
};

struct DirectAbstractDeclaratorParameter : public DirectAbstractDeclarator {
    DirectAbstractDeclarator inner;
};

struct FunctionDeclarator : public Declarator {
    Declarator decl;
    ParameterList parameters;
};

struct ParameterList {};

struct BaseParameter : public ParameterList {
    Declarator inner;
};

struct MultipleParameter : public ParameterList {
    BaseParameter inner;
    ParameterList nextParameter;
};







