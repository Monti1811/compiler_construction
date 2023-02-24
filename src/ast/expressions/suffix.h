#include "expression.h"

struct IndexExpression : public Expression {
  public:
    IndexExpression(Locatable loc, ExpressionPtr expression, ExpressionPtr index)
        : Expression(loc)
        , _expression(std::move(expression))
        , _index(std::move(index)){};

    void print(std::ostream& stream) override;

    TypePtr typecheck(ScopePtr& scope) override;
    bool isLvalue(ScopePtr&) override;

    llvm::Value* compileLValue(CompileScopePtr compile_scope) override;
    llvm::Value* compileRValue(CompileScopePtr compile_scope) override;

  private:
    // expression[index]
    ExpressionPtr _expression;
    ExpressionPtr _index;

    // signifies whether expression and index have been swapped and need to be printed vice-versa
    bool _swapped = false;
};

struct CallExpression : public Expression {
  public:
    CallExpression(Locatable loc, ExpressionPtr expression, std::vector<ExpressionPtr> arguments)
        : Expression(loc)
        , _expression(std::move(expression))
        , _arguments(std::move(arguments)){};

    void print(std::ostream& stream) override;

    TypePtr typecheck(ScopePtr& scope) override;

    llvm::Value* compileRValue(CompileScopePtr compile_scope) override;

  private:
    // expression(arguments)
    ExpressionPtr _expression;
    std::vector<ExpressionPtr> _arguments;
};

struct DotExpression : public Expression {
  public:
    DotExpression(Locatable loc, ExpressionPtr expression, Symbol ident)
        : Expression(loc)
        , _expression(std::move(expression))
        , _ident(std::move(ident)){};

    void print(std::ostream& stream) override;

    TypePtr typecheck(ScopePtr& scope) override;
    bool isLvalue(ScopePtr& scope) override;

    llvm::Value* compileLValue(CompileScopePtr compile_scope) override;
    llvm::Value* compileRValue(CompileScopePtr compile_scope) override;

  private:
    // expression.ident
    ExpressionPtr _expression;
    Symbol _ident;
};

struct ArrowExpression : public Expression {
  public:
    ArrowExpression(Locatable loc, ExpressionPtr expression, Symbol ident)
        : Expression(loc)
        , _expression(std::move(expression))
        , _ident(std::move(ident)){};

    void print(std::ostream& stream) override;

    TypePtr typecheck(ScopePtr& scope) override;
    bool isLvalue(ScopePtr& scope) override;

    llvm::Value* compileLValue(CompileScopePtr compile_scope) override;
    llvm::Value* compileRValue(CompileScopePtr compile_scope) override;

  private:
    // expression->ident
    ExpressionPtr _expression;
    Symbol _ident;
};
