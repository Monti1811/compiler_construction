#include "statement.h"

/// Block statement, e.g.:
/// ```
/// {
///    stmt1;
///    stmt2;
/// }
/// ```
struct BlockStatement : public Statement {
    BlockStatement(Locatable loc, std::vector<StatementPtr> items)
        : Statement(loc, StatementKind::ST_BLOCK)
        , _items(std::move(items)){};

    void print(std::ostream& stream) override;

    void typecheck(ScopePtr& scope) override;
    void typecheckInner(ScopePtr& inner_scope);

    void compile(CompileScopePtr compile_scope) override;

  private:
    std::vector<StatementPtr> _items;
};

/// Empty statement, i.e. just a single semicolon
/// (essentially an empty block)
struct EmptyStatement : public Statement {
    EmptyStatement(Locatable loc)
        : Statement(loc, StatementKind::ST_EMPTY){};

    void print(std::ostream& stream) override;

    void compile(CompileScopePtr compile_scope) override;

    void typecheck(ScopePtr&) override {}
};
