#include "statement.h"

#include "../expressions/expression.h"

// if (cond) then stat
// if (cond) then stat else stat
struct IfStatement : public Statement {
  public:
    IfStatement(
        Locatable loc, ExpressionPtr condition, StatementPtr then_statement, std::optional<StatementPtr> else_statement
    )
        : Statement(loc, StatementKind::ST_IF)
        , _condition(std::move(condition))
        , _then_statement(std::move(then_statement))
        , _else_statement(std::move(else_statement)){};

    IfStatement(Locatable loc, ExpressionPtr condition, StatementPtr then_statement)
        : IfStatement(loc, std::move(condition), std::move(then_statement), std::nullopt){};

    void print(std::ostream& stream) override;

    void typecheck(ScopePtr& scope) override;

    void compile(CompileScopePtr compile_scope) override;

  private:
    ExpressionPtr _condition;
    StatementPtr _then_statement;
    std::optional<StatementPtr> _else_statement;
};

// while (condition) statement
struct WhileStatement : public Statement {
  public:
    WhileStatement(Locatable loc, ExpressionPtr condition, StatementPtr statement)
        : Statement(loc, StatementKind::ST_WHILE)
        , _condition(std::move(condition))
        , _body(std::move(statement)){};

    void print(std::ostream& stream) override;

    void typecheck(ScopePtr& scope) override;

    void compile(CompileScopePtr compile_scope) override;

  private:
    ExpressionPtr _condition;
    StatementPtr _body;
};
