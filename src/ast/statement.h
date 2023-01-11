#pragma once

#include <memory>
#include <optional>
#include <vector>
#include <string>

#include "../util/diagnostic.h"
#include "../util/symbol_internalizer.h"

#include "expression.h"
#include "declarators.h"
#include "scope.h"
#include "types.h"

enum class StatementKind {
    ST_LABELED,
    ST_BLOCK,
    ST_EMPTY,
    ST_DECLARATION,
    ST_EXPRESSION,
    ST_IF,
    ST_WHILE,
    ST_JUMP
};

struct Statement {
    Statement(Locatable loc, const StatementKind kind)
        : loc(loc)
        , kind(kind) {};
    virtual ~Statement() = default;

    Locatable loc;
    const StatementKind kind;

    friend std::ostream& operator<<(std::ostream& stream, const std::unique_ptr<Statement>& stat);
    virtual void print(std::ostream& stream) = 0;

    virtual void typecheck(ScopePtr& scope) = 0;
};

typedef std::unique_ptr<Statement> StatementPtr;

// label:
// label: statement
struct LabeledStatement: public Statement {
    LabeledStatement(Locatable loc, Symbol name, StatementPtr inner)
        : Statement(loc, StatementKind::ST_LABELED)
        , _name(name)
        , _inner(std::move(inner)) {};

    void print(std::ostream& stream);

    void typecheck(ScopePtr& scope);

    private:
    Symbol _name;
    StatementPtr _inner;
};

/// Block statement, e.g.:
/// ```
/// {
///    stmt1;
///    stmt2;
/// }
/// ```
struct BlockStatement: public Statement {
    BlockStatement(Locatable loc, std::vector<StatementPtr> items)
        : Statement(loc, StatementKind::ST_BLOCK)
        , _items(std::move(items)) {};

    void print(std::ostream& stream);

    void typecheck(ScopePtr& scope);
    void typecheckInner(ScopePtr& inner_scope);

    private:
    std::vector<StatementPtr> _items;
};

struct EmptyStatement: public Statement {
    EmptyStatement(Locatable loc)
        : Statement(loc, StatementKind::ST_EMPTY) {};
    
    void print(std::ostream& stream);

    void typecheck(ScopePtr&) {}
};

// int y;
struct DeclarationStatement: public Statement {
    DeclarationStatement(Locatable loc, Declaration declaration)
        : Statement(loc, StatementKind::ST_DECLARATION)
        , _declaration(std::move(declaration)) {};

    void print(std::ostream& stream);

    void typecheck(ScopePtr& scope);

    private:
    Declaration _declaration;
};

// Just an expression disguised as a Statement
struct ExpressionStatement: public Statement {
    ExpressionStatement(Locatable loc, ExpressionPtr expr)
        : Statement(loc, StatementKind::ST_EXPRESSION)
        , _expr(std::move(expr)) {};

    void print(std::ostream& stream);

    void typecheck(ScopePtr& scope);

    private:
    ExpressionPtr _expr;
};

// if (cond) then stat
// if (cond) then stat else stat
struct IfStatement: public Statement {
    public:
    IfStatement(Locatable loc, ExpressionPtr condition, StatementPtr then_statement, std::optional<StatementPtr> else_statement)
        : Statement(loc, StatementKind::ST_IF)
        , _condition(std::move(condition))
        , _then_statement(std::move(then_statement))
        , _else_statement(std::move(else_statement)) {};
    IfStatement(Locatable loc, ExpressionPtr condition, StatementPtr then_statement)
        : IfStatement(loc, std::move(condition), std::move(then_statement), std::nullopt) {};

    void print(std::ostream& stream);

    void typecheck(ScopePtr& scope);

    private:
    ExpressionPtr _condition;
    StatementPtr _then_statement;
    std::optional<StatementPtr> _else_statement;
};

// while (condition) statement
struct WhileStatement: public Statement {
    public:
    WhileStatement(Locatable loc, ExpressionPtr condition, StatementPtr statement)
        : Statement(loc, StatementKind::ST_WHILE)
        , _condition(std::move(condition))
        , _body(std::move(statement)) {};

    void print(std::ostream& stream);

    void typecheck(ScopePtr& scope);

    private:
    ExpressionPtr _condition;
    StatementPtr _body;
};

// Parent class of jump instructions
struct JumpStatement: public Statement {
    JumpStatement(Locatable loc, Symbol name)
        : Statement(loc, StatementKind::ST_JUMP)
        , _jump_str(*name) {};

    void print(std::ostream& stream);

    void typecheck(ScopePtr&) {}

    private:
    const std::string _jump_str;
};

// goto identifier
struct GotoStatement: public JumpStatement {
    GotoStatement(Locatable loc, Symbol name, Symbol ident)
        : JumpStatement(loc, name)
        , _ident(ident) {};

    void print(std::ostream& stream);

    void typecheck(ScopePtr& scope);

    private:
    Symbol _ident;
};

// continue; (in loops)
struct ContinueStatement: public JumpStatement {
    ContinueStatement(Locatable loc, Symbol name)
        : JumpStatement(loc, name) {};
    
    void typecheck(ScopePtr& scope);
};

// break; (in loops)
struct BreakStatement: public JumpStatement {
    BreakStatement(Locatable loc, Symbol name)
        : JumpStatement(loc, name) {};

    void typecheck(ScopePtr& scope);
};

// return;
// return 1;
struct ReturnStatement: public JumpStatement {
    ReturnStatement(Locatable loc, Symbol name)
        : JumpStatement(loc, name)
        , _expr(std::nullopt) {};

    ReturnStatement(Locatable loc, Symbol name, ExpressionPtr expr)
        : JumpStatement(loc, name)
        , _expr(std::make_optional(std::move(expr))) {};

    void print(std::ostream& stream);

    void typecheck(ScopePtr& scope);

    private:
    std::optional<ExpressionPtr> _expr;
};
