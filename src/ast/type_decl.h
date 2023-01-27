#pragma once

#include <optional>
#include <memory>
#include "../util/symbol_internalizer.h"


struct Type;
typedef std::shared_ptr<Type> TypePtr;

struct TypeDecl {
    public:
    TypeDecl(std::optional<Symbol> name, TypePtr type)
        : name(name)
        , type(type) {};

    bool isAbstract() {
        return !this->name.has_value();
    }

    std::optional<Symbol> name;
    TypePtr type;
};

typedef TypeDecl FunctionParam;
typedef TypeDecl StructField;