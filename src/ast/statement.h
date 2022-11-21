#pragma once

#include <memory>
#include <optional>
#include <string>

#include "../util/symbol_internalizer.h"

#include "expression.h"
#include "declarator.h"

struct BlockItem {};

struct Statement: public BlockItem {
    Statement(Locatable loc)
        : loc(loc) {};

    Locatable loc;
};

struct LabeledStatement: public Statement {
    Symbol name;
    std::unique_ptr<Statement> inner;
};

/// Block statement, e.g.:
/// ```
/// {
///    stmt1;
///    stmt2;
/// }
/// ```
/// Also represents a null statement (`;`) if `items` is empty.
struct BlockStatement: public Statement {
    std::vector<BlockItem> items;
};

struct ExpressionStatement: public Statement {
    // expression;
    Expression expression;
};

struct IfStatement: public Statement {
    // if (condition) statement [else else_statement]
    Expression condition;
    Statement then_statement;
    std::optional<Statement> else_statement;
};

struct WhileStatement: public Statement {
    // while (condition) statement
    Expression condition;
    Statement statement;
};

struct JumpStatement: public Statement {};

struct GotoStatement: public JumpStatement {
    // goto ident
    const char** ident;
};

struct ContinueStatement: public JumpStatement {};

struct BreakStatement: public JumpStatement {};

struct ReturnStatement: public JumpStatement {
    // return expr
    std::optional<Expression> expr;
};
