#include "statement.h"

#include "../expressions/expression.h"

// Parent class of jump instructions
struct JumpStatement : public Statement {
    JumpStatement(Locatable loc, Symbol name)
        : Statement(loc, StatementKind::ST_JUMP)
        , _jump_str(*name){};

    void print(std::ostream& stream) override;

  private:
    const std::string _jump_str;
};

// goto identifier
struct GotoStatement : public JumpStatement {
    GotoStatement(Locatable loc, Symbol name, Symbol ident)
        : JumpStatement(loc, name)
        , _ident(ident){};

    void print(std::ostream& stream) override;

    void typecheck(ScopePtr& scope) override;

    void compile(CompileScopePtr compile_scope) override;

  private:
    Symbol _ident;
};

// continue; (in loops)
struct ContinueStatement : public JumpStatement {
    ContinueStatement(Locatable loc, Symbol name)
        : JumpStatement(loc, name){};

    void typecheck(ScopePtr& scope) override;

    void compile(CompileScopePtr compile_scope) override;
};

// break; (in loops)
struct BreakStatement : public JumpStatement {
    BreakStatement(Locatable loc, Symbol name)
        : JumpStatement(loc, name){};

    void typecheck(ScopePtr& scope) override;

    void compile(CompileScopePtr compile_scope) override;
};

// return;
// return 1;
struct ReturnStatement : public JumpStatement {
    ReturnStatement(Locatable loc, Symbol name)
        : JumpStatement(loc, name)
        , _expr(std::nullopt){};

    ReturnStatement(Locatable loc, Symbol name, ExpressionPtr expr)
        : JumpStatement(loc, name)
        , _expr(std::make_optional(std::move(expr))){};

    void print(std::ostream& stream) override;

    void typecheck(ScopePtr& scope) override;

    void compile(CompileScopePtr compile_scope) override;

  private:
    std::optional<ExpressionPtr> _expr;
};
