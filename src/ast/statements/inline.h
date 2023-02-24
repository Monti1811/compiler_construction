#include "statement.h"

#include "../declaration.h"
#include "../expressions/expression.h"

// label:
// label: statement
struct LabeledStatement : public Statement {
    LabeledStatement(Locatable loc, Symbol name, StatementPtr inner)
        : Statement(loc, StatementKind::ST_LABELED)
        , _name(name)
        , _inner(std::move(inner)){};

    void print(std::ostream& stream) override;

    void typecheck(ScopePtr& scope) override;

    void compile(CompileScopePtr compile_scope) override;

  private:
    Symbol _name;
    StatementPtr _inner;
};


// int y;
struct DeclarationStatement : public Statement {
    DeclarationStatement(Locatable loc, Declaration declaration)
        : Statement(loc, StatementKind::ST_DECLARATION)
        , _declaration(std::move(declaration)){};

    void print(std::ostream& stream) override;

    void compile(CompileScopePtr compile_scope) override;

    void typecheck(ScopePtr& scope) override;

  private:
    Declaration _declaration;
};

// Just an expression disguised as a Statement
struct ExpressionStatement : public Statement {
    ExpressionStatement(Locatable loc, ExpressionPtr expr)
        : Statement(loc, StatementKind::ST_EXPRESSION)
        , _expr(std::move(expr)){};

    void print(std::ostream& stream) override;

    void compile(CompileScopePtr compile_scope) override;

    void typecheck(ScopePtr& scope) override;

  private:
    ExpressionPtr _expr;
};
