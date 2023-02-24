#pragma once

#include <memory>

#include "../../util/diagnostic.h"

#include "../compile_scope.h"
#include "../scope.h"

enum class StatementKind { ST_LABELED, ST_BLOCK, ST_EMPTY, ST_DECLARATION, ST_EXPRESSION, ST_IF, ST_WHILE, ST_JUMP };

struct Statement {
    Statement(Locatable loc, const StatementKind kind)
        : loc(loc)
        , kind(kind){};

    friend std::ostream& operator<<(std::ostream& stream, const std::unique_ptr<Statement>& stat);
    virtual void print(std::ostream& stream) = 0;

    virtual void typecheck(ScopePtr& scope) = 0;

    virtual void compile(CompileScopePtr compile_scope) = 0;

    Locatable loc;
    const StatementKind kind;
};

typedef std::unique_ptr<Statement> StatementPtr;
